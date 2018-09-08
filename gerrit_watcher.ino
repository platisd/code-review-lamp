#include <vector>
#include <map>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "credentials.h"

struct RGBColor {
  public:
    RGBColor(int r = 0, int g = 0, int b = 0) : red{r}, green{g}, blue{b} {}
    int red;
    int green;
    int blue;
};

const auto NEOPIXEL_PIN = 15;
const auto NEOPIXEL_RING_SIZE = 16;
const auto CONNECTION_RETRIES = 20;
const auto OPEN_REVIEWS_QUERY = "/a/changes/?q=status:open+is:reviewer";
const String CHANGES_ENDPOINT = GERRIT_URL + "/a/changes/";
const auto REVIEWERS = "/reviewers/";
const auto DELETE = "/delete";
const auto ALL_REVIEWS_ASSIGNED_URL = GERRIT_URL + OPEN_REVIEWS_QUERY;
const auto GERRIT_REVIEW_NUMBER_ATTRIBUTE = "_number";
const auto GERRIT_REVIEW_APPROVAL_ATTRIBUTE = "Code-Review";
const auto GERRIT_REVIEW_OWNERID_ATTRIBUTE = "_account_id";
const auto WAIT_FOR_GERRIT_RESPONSE = 500;
const auto ENOUGH_CONDUCTED_REVIEWS = 1;

const RGBColor KINDA_ORANGE (216, 81, 23);
const RGBColor MELLOW_YELLOW (196, 178, 41);
const RGBColor ALIEN_GREEN (174, 249, 52);
const RGBColor COOL_CYAN (43, 239, 177);
const RGBColor GREEK_BLUE (42, 62, 239);
const RGBColor GOTH_PURPLE (244, 68, 235);
// Maps gerrit account ids with lamp colors
std::map<String, RGBColor> LAMP_COLORS {{"1000000", KINDA_ORANGE}, {"1000002", MELLOW_YELLOW}, {"1000003", ALIEN_GREEN}, {"1000004", COOL_CYAN}, {"1000005", GREEK_BLUE}, {"1000006", GOTH_PURPLE} };

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
   @param getReviewersUrl   The URL of the reviewers endpoint
   @param gerritUser        The Gerrit username to be removed from the review
*/
void removeFromReview(const String& getReviewersUrl, const String& gerritUser) {
  HTTPClient http;
  Serial.println(getReviewersUrl + gerritUser + DELETE);
  http.begin(getReviewersUrl + gerritUser + DELETE);
  http.setAuthorization(GERRIT_USERNAME, GERRIT_HTTP_PASSWORD);
  auto httpCode = http.POST(""); // No payload needed
  http.end();

  if (httpCode < 0 || httpCode != HTTP_CODE_NO_CONTENT) {
    Serial.printf("[%s] POST failed, code: %s\n", __FUNCTION__, http.errorToString(httpCode).c_str());
    indicateError();
  }
}

/**
   @param url    The URL to execute a GET request
   @param key    The key to look inside the incoming JSON stream
   @return       A list with all the values of the specific key
*/
std::vector<String> getStreamAttribute(const String& url, const String& key) {
  HTTPClient http;
  http.begin(url);
  http.setAuthorization(GERRIT_USERNAME, GERRIT_HTTP_PASSWORD);
  auto httpCode = http.GET();

  if (httpCode < 0 || httpCode != HTTP_CODE_OK) {
    Serial.printf("[%s] GET failed, code: %s\n", __FUNCTION__, http.errorToString(httpCode).c_str());
    indicateError();
  }
  delay(WAIT_FOR_GERRIT_RESPONSE);

  auto documentLength = http.getSize();
  auto stream = http.getStream();

  std::vector<String> keyValues;
  if (http.connected() && (documentLength > 0 || documentLength == -1)) {
    while (stream.available()) {
      // Parse the value of the key when found
      String line = stream.readStringUntil('\n');
      line.trim();
      line.replace("\"", "");
      // Clear out unnecessary characters
      if (line.startsWith(key)) {
        line.replace(key, "");
        line.replace(",", "");
        line.replace(":", "");
        line.trim();
        // By now it should include just the information we are interested in
        keyValues.push_back(line);
      }
    }
  }

  if (keyValues.empty()) {
    Serial.printf("Error - Key not found: %s\n", key.c_str());
    indicateError();
  }
  http.end();

  return keyValues;
}

void connectToWifi() {
  // Set WiFi to station mode and disconnect from an AP
  // if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Try to connect to the internet.
  WiFi.begin(INTERNET_SSID, PASSWORD);
  auto attemptsLeft = CONNECTION_RETRIES;
  Serial.print("Connecting");
  while ((WiFi.status() != WL_CONNECTED) && (--attemptsLeft > 0)) {
    delay(500); // Wait a bit before retrying
    Serial.print(".");
  }

  if (attemptsLeft <= 0) {
    Serial.println(" Connection error!");
    indicateError();
  }
  Serial.println(" Connection success");
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  connectToWifi();

  // Get all reviews assigned to our user
  Serial.println("Getting all reviews assigned to us");
  auto reviews = getStreamAttribute(ALL_REVIEWS_ASSIGNED_URL, GERRIT_REVIEW_NUMBER_ATTRIBUTE);
  for (auto& review : reviews) {
    // Get all approvals for the specific review
    Serial.printf("Getting all approvals for review %s\n", review.c_str());
    auto getChangeUrl = CHANGES_ENDPOINT + review;
    auto getReviewersUrl = CHANGES_ENDPOINT + review + REVIEWERS;
    auto approvals = getStreamAttribute(getReviewersUrl, GERRIT_REVIEW_APPROVAL_ATTRIBUTE);
    auto finishedReviews = 0;
    for (auto& approval : approvals) {
      Serial.println(approval);
      if (approval != "0") {
        finishedReviews++;
      }
    }

    if (finishedReviews > ENOUGH_CONDUCTED_REVIEWS) {
      Serial.printf("We got enough reviews in %s, removing ourselves\n", review.c_str());
      removeFromReview(getReviewersUrl, GERRIT_USERNAME);
    } else {
      auto ownerId = getStreamAttribute(getChangeUrl, GERRIT_REVIEW_OWNERID_ATTRIBUTE).front();
      Serial.printf("Owner ID: %s\n", ownerId.c_str());
    }
  }

}

void loop() {

}
