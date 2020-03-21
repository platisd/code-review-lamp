#include "Colors.h"
#include "Date.h"
#include "Optional.h"
#include "Utilities.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <vector>

using namespace corona_lamp;

const auto kSsid                    = "your-ssid";
const auto kPassword                = "your-password";
const auto kNeopixelPin             = 15;
const auto kNeopixelRingSize        = 16;
const auto kErrorBlinkInterval      = 250UL;
const auto kReconnectTimeout        = 100UL;
const auto kRetryConnectionInterval = 500UL;
const auto kConnectionRetries       = 20;
const auto kOneMinute               = 1000UL * 60UL;
const auto kWifiReconnectInterval   = 30000UL;

const auto kCountry               = "sweden";
const char kFingerprint[] PROGMEM = "07 62 08 46 01 4E 07 CB 68 AB 12 53 A8 5E 6F 7E D4 D3 4E 20";

const HSVColor KINDA_ORANGE(10, 100, 100);
const HSVColor MELLOW_YELLOW(40, 100, 100);
const HSVColor ALIEN_GREEN(100, 100, 100);
const HSVColor ALMOST_WHITE(293, 4, 70);
const HSVColor GREEK_BLUE(227, 100, 100);
const HSVColor GOTH_PURPLE(315, 100, 100);
const HSVColor BLOOD_RED(0, 100, 100);

Adafruit_NeoPixel ring(kNeopixelRingSize, kNeopixelPin, NEO_GRB + NEO_KHZ800);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/**
   Display an error pattern on the neopixels
   @param neopixels The neopixels to display the error
*/
void indicateError(Adafruit_NeoPixel& neopixels)
{
    // Blink red LEDs sequentially to indicate an error
    for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++)
    {
        neopixels.setPixelColor(pixel, 200, 0, 0);
        neopixels.show();
        delay(kErrorBlinkInterval);
        neopixels.setPixelColor(pixel, 0, 0, 0);
        neopixels.show();
        delay(kErrorBlinkInterval);
    }
}

/**
   (Re)connects the module to WiFi
*/
void connectToWifi()
{
    // Set WiFi to station mode & disconnect from an AP if previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(kReconnectTimeout);

    // Try to connect to the internet
    WiFi.begin(kSsid, kPassword);
    auto attemptsLeft = kConnectionRetries;
    Serial.print("Connecting");
    while ((WiFi.status() != WL_CONNECTED) && (--attemptsLeft > 0))
    {
        delay(kRetryConnectionInterval); // Wait a bit before retrying
        Serial.print(".");
    }

    if (attemptsLeft <= 0)
    {
        Serial.println(" Connection error!");
        indicateError(ring);
    }
    else
    {
        Serial.println(" Connection success");
    }
}

std::vector<Date> getLatestDatePeriod(int days)
{
    static const auto secondsInDay = 86400UL;
    timeClient.update();
    // Get yesterday's data to ensure we are never too early to fetch data for the day
    const auto yesterday = timeClient.getEpochTime() - secondsInDay;
    std::vector<Date> period;

    for (auto i = days - 1; i >= 0; i--)
    {

        std::time_t t(yesterday - (i * secondsInDay));
        const auto now = std::localtime(&t);
        period.emplace_back(Date(now->tm_year + 1900, now->tm_mon + 1, now->tm_mday));
    }

    return period;
}

Optional<int> getInfectionsUntil(const Date& date, const char* country)
{
    WiFiClientSecure client;
    client.setFingerprint(kFingerprint);

    static const auto host = "covid-api.quintessential.gr";
    static const auto port = 443;

    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return Nullopt;
    }

    static const String baseUrl = "/data/custom?";
    const String query          = baseUrl + String("country=") + String(country) + String("&date=") + String(date);

    const auto getRequest = String("GET ") + query + " HTTP/1.1\r\n" + "Host: " + host + "\r\n"
                            + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n";
    client.print(getRequest);

    // Go through the headers before we can deserialize the JSON
    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }

    DynamicJsonDocument doc(600);
    deserializeJson(doc, client);
    const auto root               = doc.as<JsonArray>();
    const auto payload            = root[0];
    static const auto infectedKey = "Confirmed";

    Serial.print("Date: ");
    Serial.print(date);
    Serial.print("\t\t");

    if (!payload.containsKey(infectedKey))
    {
        Serial.println("No data");
        return Nullopt;
    }

    Optional<int> infections = payload[infectedKey].as<int>();
    Serial.println(infections.value());

    return infections;
}

std::vector<Optional<int>> getTotalInfectionsPerDayFor(const std::vector<Date>& period)
{
    std::vector<Optional<int>> totalInfectionsPerDay;

    for (const auto& date : period)
    {
        totalInfectionsPerDay.emplace_back(getInfectionsUntil(date, kCountry));
    }

    return totalInfectionsPerDay;
}

std::vector<Optional<float>> getInfectionGrowthRates(const std::vector<Optional<int>>& infections)
{
    std::vector<Optional<float>> infectionGrowthRates(infections.size() - 1);

    for (auto i = 1; i < infections.size(); i++)
    {
        const auto indexToFill = i - 1;
        if (!!infections.at(i))
        {
            if (!!infections.at(i - 1))
            {
                const auto currentInfection  = infections.at(i).value();
                const auto previousInfection = infections.at(i - 1).value();
                const auto infectionDelta    = currentInfection - previousInfection;
                const auto noGrowth          = infectionDelta == 0;
                const Optional<float> growthRate
                    = noGrowth ? 0.0f : static_cast<float>(infectionDelta) / static_cast<float>(previousInfection);
                infectionGrowthRates[indexToFill] = growthRate;
            }
            else
            {
                // If there are no previous data, assume an 100% growth rate
                infectionGrowthRates[indexToFill] = Optional<float>(1);
            }
        }
        else
        {
            infectionGrowthRates[indexToFill] = Nullopt;
        }
    }

    return infectionGrowthRates;
}

std::vector<HSVColor> getGrowthColors(const std::vector<Optional<float>>& growthRates)
{
    static const auto baseColor = MELLOW_YELLOW;
    std::vector<HSVColor> colors;

    for (const auto& growthRate : growthRates)
    {
        auto color = baseColor;
        if (!growthRate)
        {
            color = HSVColor(0, 0, 0);
        }
        else
        {
            static const auto okGrowthRate = 0.05f;
            const auto rate                = constrain(growthRate.value(), 0.0f, 1.0f);
            const auto hueDrift = rate <= okGrowthRate ? getMap<float>(rate, 0.0f, okGrowthRate, 100.0f, 60.0f)
                                                       : getMap<float>(rate, okGrowthRate, 1.0f, 20.0f, 0.0f);
            color.hue = hueDrift;
            Serial.print("Growth: ");
            Serial.print(growthRate.value());
        }
        Serial.print("\t\tHue: ");
        Serial.println(color.hue);
        colors.emplace_back(color);
    }

    return colors;
}

void showClockEffect(Adafruit_NeoPixel& neopixels, const std::vector<HSVColor>& colors)
{
    assert(neopixels.numPixels() <= colors.size());
    for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++)
    {
        const auto color = colors.at(pixel).toRGB();
        neopixels.setPixelColor(pixel, color.red, color.green, color.blue);
        neopixels.show();
        delay(100);
    }
}

void dimDown(Adafruit_NeoPixel& neopixels, const std::vector<HSVColor>& colors)
{
    assert(neopixels.numPixels() <= colors.size());
    for (auto value = 100; value >= 0; value--)
    {
        for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++)
        {
            const auto color = HSVColor(colors.at(pixel).hue, colors.at(pixel).saturation, value).toRGB();
            neopixels.setPixelColor(pixel, color.red, color.green, color.blue);
        }
        neopixels.show();
        delay(100);
    }
}

void dimUp(Adafruit_NeoPixel& neopixels, const std::vector<HSVColor>& colors)
{
    assert(neopixels.numPixels() <= colors.size());
    for (auto value = 0; value <= 100; value++)
    {
        for (auto pixel = 0; pixel < neopixels.numPixels(); pixel++)
        {
            const auto color = HSVColor(colors.at(pixel).hue, colors.at(pixel).saturation, value).toRGB();
            neopixels.setPixelColor(pixel, color.red, color.green, color.blue);
        }
        neopixels.show();
        delay(100);
    }
}

void setup()
{
    Serial.begin(115200);
    ring.begin();
    ring.show(); // Initialize all pixels to 'off'
    connectToWifi();
    timeClient.begin();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWifi();
        delay(kWifiReconnectInterval);
    }
    else
    {
        const auto daysToStudy          = kNeopixelRingSize + 1; // Number of pixels available + previous day
        const auto period               = getLatestDatePeriod(daysToStudy);
        const auto infectionsPerDay     = getTotalInfectionsPerDayFor(period);
        const auto infectionGrowthRates = getInfectionGrowthRates(infectionsPerDay);
        const auto colors               = getGrowthColors(infectionGrowthRates);
        for (auto hours = 0; hours < 12; hours++)
        {
            showClockEffect(ring, colors);
            for (auto minutes = 0; hours < 60; minutes++)
            {
                dimDown(ring, colors);
                dimUp(ring, colors);
                delay(kOneMinute);
            }
        }
    }
}
