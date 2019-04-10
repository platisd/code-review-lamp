#include <vector>
#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Keep your project specific credentials in a non-version controlled
// `credentials.h` file and set the `NO_CREDENTIALS_HEADER` to false.
#define NO_CREDENTIALS_HEADER false
#if NO_CREDENTIALS_HEADER == true
const auto INTERNET_SSID = "your_ssid";
const auto PASSWORD = "your_password";
const auto GITHUB_USERNAME = "your_GITHUB_USERNAME";
const auto GITHUB_OAUTH2_TOKEN = "your_GITHUB_OAUTH2_TOKEN";
const auto INTERMEDIATE_SERVER = "http://your.flask.server/";
#else
#include "credentials.h"
#endif

enum class Effect {PULSE, RADAR, COOL_RADAR};
const auto MY_EFFECT = Effect::RADAR;

struct RGBColor {
  RGBColor(int r = 0, int g = 0, int b = 0) : red{r}, green{g}, blue{b} {}
  int red;
  int green;
  int blue;
};

struct HSVColor {
  /**
    @param h    Hue ranged          [0,360)
    @param s    Saturation ranged   [0,100]
    @param v    Value ranged        [0,100]
  */
  HSVColor(int h = 0, int s = 0, int v = 0) : hue{h}, saturation{s}, value{v} {}
  int hue;
  int saturation;
  int value;

  /**
    Converts HSV colors to RGB that can be used for Neopixels
    so that we can adjust the brightness of the colors.
    Code adapted from: https://stackoverflow.com/a/14733008

    @param hsv  The color in HSV format to convert
    @return     The equivalent color in RGB
  */
  RGBColor toRGB() const {
    // Scale the HSV values to the expected range
    auto rangedHue = map(hue, 0, 359, 0, 255);
    auto rangedSat = map(saturation, 0, 100, 0, 255);
    auto rangedVal = map(value, 0, 100, 0, 255);

    if (rangedSat == 0) {
      return {rangedVal, rangedVal, rangedVal};
    }

    auto region = rangedHue / 43;
    auto remainder = (rangedHue - (region * 43)) * 6;

    auto p = (rangedVal * (255 - rangedSat)) >> 8;
    auto q = (rangedVal * (255 - ((rangedSat * remainder) >> 8))) >> 8;
    auto t = (rangedVal * (255 - ((rangedSat * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
      case 0:
        return {rangedVal, t, p};
      case 1:
        return {q, rangedVal, p};
      case 2:
        return {p, rangedVal, t};
      case 3:
        return {p, q, rangedVal};
      case 4:
        return {t, p, rangedVal};
      default:
        return {rangedVal, p, q};
    }
  }
};

const auto NEOPIXEL_PIN = 15;
const auto NEOPIXEL_RING_SIZE = 16;
const auto DIM_WINDOW = 10000UL;
const auto CHECK_FOR_REVIEWS_INTERVAL = 20000UL;
const auto ERROR_BLINK_INTERVAL = 250UL;
const auto WAIT_FOR_GITHUB_RESPONSE = 50UL;
const auto RECONNECT_TIMEOUT = 100UL;
const auto RETRY_CONNECTION_INTERVAL = 500UL;
const auto CONNECTION_RETRIES = 20;
const auto REQUEST_URL = INTERMEDIATE_SERVER + String(GITHUB_USERNAME) + "/" + String(GITHUB_OAUTH2_TOKEN);

const HSVColor KINDA_ORANGE (10, 100, 100);
const HSVColor MELLOW_YELLOW (30, 100, 100);
const HSVColor ALIEN_GREEN (100, 100, 100);
const HSVColor ALMOST_WHITE (293, 4, 70);
const HSVColor GREEK_BLUE (227, 100, 100);
const HSVColor GOTH_PURPLE (315, 100, 100);
const HSVColor BLOOD_RED (0, 100, 100);

Adafruit_NeoPixel ring(NEOPIXEL_RING_SIZE, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

/**
   Maps Gerrit user IDs to lamp colors, adapt accordingly.
   @param userId  Gerrit user ID
   @return The HSV color to match the specific user
*/
HSVColor toColor(const String& color) {
  if (color == "yellow") {
    return MELLOW_YELLOW;
  } else if (color == "green") {
    return ALIEN_GREEN;
  } else if (color == "purple") {
    return GOTH_PURPLE;
  } else if (color == "orange") {
    return KINDA_ORANGE;
  } else if (color == "red") {
    return BLOOD_RED;
  } else if (color == "blue") {
    return GREEK_BLUE;
  } else {
    return ALMOST_WHITE;
  }
}

/**
   Display an error pattern on the neopixels
   @param neopixels The neopixels to display the error
*/
void indicateError(Adafruit_NeoPixel& neopixels) {
  // Blink red LEDs sequentially to indicate an error
  for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++) {
    neopixels.setPixelColor(pixel, 200, 0, 0);
    neopixels.show();
    delay(ERROR_BLINK_INTERVAL);
    neopixels.setPixelColor(pixel, 0, 0, 0);
    neopixels.show();
    delay(ERROR_BLINK_INTERVAL);
  }
}

/**
   (Re)connects the module to WiFi
*/
void connectToWifi() {
  // Set WiFi to station mode & disconnect from an AP if previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(RECONNECT_TIMEOUT);

  // Try to connect to the internet
  WiFi.begin(INTERNET_SSID, PASSWORD);
  auto attemptsLeft = CONNECTION_RETRIES;
  Serial.print("Connecting");
  while ((WiFi.status() != WL_CONNECTED) && (--attemptsLeft > 0)) {
    delay(RETRY_CONNECTION_INTERVAL); // Wait a bit before retrying
    Serial.print(".");
  }

  if (attemptsLeft <= 0) {
    Serial.println(" Connection error!");
    indicateError(ring);
  } else {
    Serial.println(" Connection success");
  }
}

/**
    Get all open reviews that our gerrit user has been assigned to,
    then figure out whether enough developers have code-reviewed
    each review. If this has not happened, return a color
    (mapped to a developer) for every un-reviewed review.
    Once a review has been reviewed adequately, remove ourselves
    from it.

    @return HSV colors for every unfinished code review
*/
std::vector<HSVColor> getColorsForUnfinishedReviews() {
  HTTPClient http;
  http.begin(REQUEST_URL);
  auto httpCode = http.GET();

  if (httpCode < 0 || httpCode != HTTP_CODE_OK) {
    Serial.printf("[%s] GET failed with code '%s'\r\n", __FUNCTION__, http.errorToString(httpCode).c_str());
    http.end();
    return {};
  }
  delay(WAIT_FOR_GITHUB_RESPONSE);

  auto documentLength = http.getSize();
  auto stream = http.getStream();

  std::vector<HSVColor> colors;
  if (http.connected() && (documentLength > 0 || documentLength == -1)) {
    while (stream.available()) {
      String color = stream.readStringUntil(',');
      colors.push_back(toColor(color));
    }
  }

  if (colors.empty()) {
    Serial.println("Warning - Colors not found!");
  }
  http.end();

  return colors;
}

/**
   Set the specified RGB color to all the pixels

   @param neopixels The neopixel structure to set color
   @param rgbColor  The RGB color to set the pixels
*/

void setAllPixelColor(Adafruit_NeoPixel& neopixels, RGBColor& rgbColor) {
  switch (MY_EFFECT) {
    case Effect::RADAR:
      setRadarEffect(neopixels, rgbColor);
      break;
    case Effect::PULSE:
      setPulseEffect(neopixels, rgbColor);
      break;
    case Effect::COOL_RADAR:
      setCoolRadarEffect(neopixels, rgbColor);
      break;
    default:
      setRadarEffect(neopixels, rgbColor);
      break;
  }
}

/**
   Perform some radar effect
   @param neopixels The neopixel structure to set color
   @param rgbColor  The RGB color to set the pixels
*/
void setRadarEffect(Adafruit_NeoPixel& neopixels, RGBColor& rgbColor) {
  const auto pixels = neopixels.numPixels();
  static const auto SLICES = 3;
  static uint8_t startingPixel = 0;

  startingPixel++; // Does not matter if it rotates back to 0

  // Slice the lamp in parts where the first and brightest one is our radar effect
  // while the rest have a progressively dimmer color.
  for (auto slice = 0; slice < SLICES; slice++) {
    for (auto pixel = slice * pixels / SLICES + startingPixel; pixel < (slice + 1) * pixels / SLICES + startingPixel; pixel++) {
      neopixels.setPixelColor(pixel % pixels, rgbColor.red, rgbColor.green, rgbColor.blue);
    }

    rgbColor.red   /= 2;
    rgbColor.green /= 2;
    rgbColor.blue  /= 2;
  }
}

/**
   Perform some cool radar effect
   @param neopixels The neopixel structure to set color
   @param rgbColor  The RGB color to set the pixels
*/
void setCoolRadarEffect(Adafruit_NeoPixel& neopixels, RGBColor& rgbColor) {
  const auto pixels = neopixels.numPixels();
  static uint8_t startingPixel = 0;

  startingPixel++; // Does not matter if it rotates back to 0

  for (auto pixel = 0 + startingPixel; pixel < 3 * pixels / 5 + startingPixel; pixel++) {
    neopixels.setPixelColor(pixel % pixels, rgbColor.red, rgbColor.green, rgbColor.blue);
  }

  RGBColor rgb1;
  rgb1.red   = rgbColor.green;
  rgb1.green = rgbColor.blue;
  rgb1.blue  = rgbColor.red;

  for (auto pixel = 3 * pixels / 5 + startingPixel; pixel < 4 * pixels / 5 + startingPixel; pixel++) {
    neopixels.setPixelColor(pixel % pixels, rgb1.red, rgb1.green, rgb1.blue);
  }

  RGBColor rgb2;
  rgb2.red   = rgbColor.blue;
  rgb2.green = rgbColor.red;
  rgb2.blue  = rgbColor.green;

  for (auto pixel = 4 * pixels / 5 + startingPixel; pixel < pixels + startingPixel; pixel++) {
    neopixels.setPixelColor(pixel % pixels, rgb2.red, rgb2.green, rgb2.blue);
  }
}

/**
  Perform some pulse effect
   @param neopixels The neopixel structure to set color
   @param rgbColor  The RGB color to set the pixels
*/
void setPulseEffect(Adafruit_NeoPixel& neopixels, RGBColor& rgbColor) {
  for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++) {
    neopixels.setPixelColor(pixel, rgbColor.red, rgbColor.green, rgbColor.blue);
  }
}

/**
   Dim for all the supplied colors throughout the specified window
   @param neopixels The neopixel structure to dim
   @param hsvColors The HSV colors to be dimmed
*/
void dimWithColors(Adafruit_NeoPixel& neopixels, std::vector<HSVColor>& hsvColors) {
  if (hsvColors.empty()) {
    Serial.println("All code is reviewed, good job");
    return;
  }
  // Dim every color within the designated time window
  // The effect we are after is the more unfinished reviews
  // the faster the neopixels will dim
  auto timeSlotForEachColor = DIM_WINDOW / hsvColors.size();
  for (const auto& hsvColor : hsvColors) {
    auto rgb = hsvColor.toRGB();
    // The time slot has to be evenly divided among the intervals necessary
    // to dim it all the way up and down
    auto dimInterval = (timeSlotForEachColor / hsvColor.value) / 2;
    // Dim up every pixel for the current color
    for (auto intensity = 0; intensity < hsvColor.value; intensity++) {
      // Get the RGB value of the currently dimmed HSV color
      auto rgbColor = HSVColor(hsvColor.hue, hsvColor.saturation, intensity).toRGB();
      setAllPixelColor(neopixels, rgbColor);
      neopixels.show();
      delay(dimInterval);
    }

    // Dim down every pixel for the current color
    for (auto intensity = hsvColor.value; intensity >= 0; intensity--) {
      // Get the RGB value of the currently dimmed HSV color
      auto rgbColor = HSVColor(hsvColor.hue, hsvColor.saturation, intensity).toRGB();
      setAllPixelColor(neopixels, rgbColor);
      neopixels.show();
      delay(dimInterval);
    }
  }
}

void setup() {
  Serial.begin(9600);
  ring.begin();
  ring.show(); // Initialize all pixels to 'off'
  connectToWifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  } else {
    auto hsvColors = getColorsForUnfinishedReviews();
    dimWithColors(ring, hsvColors);
  }
  delay(CHECK_FOR_REVIEWS_INTERVAL);
}
