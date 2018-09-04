#include <vector>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "credentials.h"

const auto CONNECTION_RETRIES = 20;
const auto OPEN_REVIEWS_QUERY = "/a/changes/?q=status:open+is:reviewer";
const auto CHANGES_ENDPOINT = "/a/changes/";
const auto REVIEWERS = "/reviewers";
// The first five bytes received from gerrit are the "magic prefix" which
// should be ignored
const auto GERRIT_MAGIC_PREFIX_SIZE = 5; // )]}'\n
char magicPrefixBuffer[5]; // A buffer for consuming the magic prefix

ESP8266WiFiClass wifi;

/**
   Block and indicate an error to the user
*/
void indicateError() {
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

/**
   Sends a GET request to the specified URI and returns the
   response as a JSON Array
*/
JsonArray getJsonArrayFromGET(const String& url) {
  HTTPClient http;
  http.begin(url);
  http.setAuthorization("nikosp", "RRXWEQA9aguvXlsES2XA0ayMbiJ2JtpHety0r3LeJw");
  auto httpCode = http.GET();

  if (httpCode < 0 || httpCode != HTTP_CODE_OK) {
    Serial.printf("GET failed, code: %s\n", http.errorToString(httpCode).c_str());
    indicateError();
  }
  delay(1000); // Wait for the response to be received

  auto documentLength = http.getSize();
  auto stream = http.getStream();

  // Create dynamically a payload buffer
  if (stream.available() <= GERRIT_MAGIC_PREFIX_SIZE) {
    Serial.println("Error: No large enough response received from Gerrit");
    indicateError();
  }

  // read all data from server
  if (http.connected() && (documentLength > 0 || documentLength == -1)) {
    stream.readBytes(magicPrefixBuffer, GERRIT_MAGIC_PREFIX_SIZE); // Consume the magic prefix bytes
  }

  DynamicJsonDocument doc(stream.available());
  deserializeJson(doc, stream);
  http.end();

  return doc.as<JsonArray>();
}

void connectToWifi() {
  // Set WiFi to station mode and disconnect from an AP
  // if it was previously connected.
  wifi.mode(WIFI_STA);
  wifi.disconnect();
  delay(100);

  // Try to connect to the internet.
  wifi.begin(INTERNET_SSID, PASSWORD);
  auto attemptsLeft = CONNECTION_RETRIES;
  Serial.print("Connecting");
  while ((wifi.status() != WL_CONNECTED) && (--attemptsLeft > 0)) {
    delay(500); // Wait a bit before retrying
    Serial.print(".");
  }

  if (attemptsLeft <= 0) {
    Serial.println(" Connection error!");
    indicateError();
  }
  Serial.println(" Connection success");
}

std::vector<String> getOpenReviews() {
  std::vector<String> openReviews;
  auto reviews = getJsonArrayFromGET(String(GERRIT_URL + OPEN_REVIEWS_QUERY));

  for (auto& review : reviews) {
    String reviewNumber = review["_number"];
    openReviews.push_back(reviewNumber);
  }

  return openReviews;
}

int getFinishedReviews(const String& review) {
  auto finishedReviews = 0;
  auto reviews = getJsonArrayFromGET(String(GERRIT_URL + CHANGES_ENDPOINT + review + REVIEWERS));

  for (auto& review : reviews) {
    auto codeReview = review["approvals"]["Code-Review"];
    if (codeReview != " 0") {
      finishedReviews++;
    }
  }

  return finishedReviews;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  connectToWifi();

  auto reviews = getOpenReviews();
  for (auto& review : reviews) {
    // Get all reviewers and their ratings
    Serial.println(review);
    auto finishedReviews = getFinishedReviews(review);
    Serial.printf("Finished reviews %d:\n", finishedReviews);

    if (finishedReviews > 0) {
      for (int i = 0; i < 20; i++) {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(20);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        delay(20);                       // wait for a second
      }
    } else {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(1000);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        delay(1000);                       // wait for a second
    }
  }

}

void loop() {

}
