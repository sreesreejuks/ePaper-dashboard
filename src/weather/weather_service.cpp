#include "weather_service.h"

#include "../config/config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

namespace {
// WMO weather codes (https://open-meteo.com/en/docs) collapsed into the
// short strings WeatherPanel displays in WeatherData::condition.
const char *conditionForCode(int code) {
    if (code == 0) return "Clear Sky";
    if (code == 1) return "Mostly Clear";
    if (code == 2) return "Partly Cloudy";
    if (code == 3) return "Overcast";
    if (code == 45 || code == 48) return "Fog";
    if (code >= 51 && code <= 55) return "Drizzle";
    if (code == 56 || code == 57) return "Freezing Drizzle";
    if (code >= 61 && code <= 65) return "Rain";
    if (code == 66 || code == 67) return "Freezing Rain";
    if (code >= 71 && code <= 75) return "Snow";
    if (code == 77) return "Snow Grains";
    if (code >= 80 && code <= 82) return "Rain Showers";
    if (code == 85 || code == 86) return "Snow Showers";
    if (code == 95) return "Thunderstorm";
    if (code == 96 || code == 99) return "Thunderstorm w/ Hail";
    return "Unknown";
}

bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WEATHER_FETCH_TIMEOUT_MS) {
        delay(250);
    }
    return WiFi.status() == WL_CONNECTED;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}
} // namespace

bool WeatherService::fetch(WeatherData &out) {
    bool ok = false;
    if (!connectWiFi()) {
        Serial.println("[Weather] WiFi connect failed/timed out.");
    } else {
        char url[256];
        snprintf(url, sizeof(url),
                 "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
                 "&current=temperature_2m,relative_humidity_2m,weather_code"
                 "&daily=temperature_2m_max,temperature_2m_min&timezone=Asia%%2FKolkata",
                 WEATHER_LATITUDE, WEATHER_LONGITUDE);

        WiFiClientSecure client;
        client.setInsecure(); // public read-only endpoint; skip cert pinning

        HTTPClient http;
        http.setTimeout(WEATHER_FETCH_TIMEOUT_MS);
        // Open-Meteo's response is chunked transfer-encoded over HTTP/1.1,
        // and http.getStream() hands ArduinoJson the raw chunk framing
        // instead of dechunked JSON, which deserializeJson() rejects as
        // InvalidInput. Requesting HTTP/1.0 gets a plain Content-Length body
        // instead.
        http.useHTTP10(true);
        if (!http.begin(client, url)) {
            Serial.println("[Weather] HTTPClient::begin() failed (bad URL?).");
        } else {
            int status = http.GET();
            if (status != HTTP_CODE_OK) {
                // Negative status = HTTPClient-level failure (DNS/TLS/connect,
                // see HTTPC_ERROR_* in HTTPClient.h); positive = a real HTTP
                // status code (404, 500, ...) returned by the server.
                Serial.printf("[Weather] HTTP GET failed, status=%d\n", status);
            } else {
                // Sized generously: the real payload includes current_units/
                // daily_units alongside current/daily, and ArduinoJson's pool
                // overhead per key/value adds up fast -- 768 measured too
                // small in practice and silently failed to parse.
                StaticJsonDocument<2048> doc;
                DeserializationError err = deserializeJson(doc, http.getStream());
                if (err) {
                    Serial.printf("[Weather] JSON parse failed: %s\n", err.c_str());
                } else {
                    JsonObject current = doc["current"];
                    JsonObject daily = doc["daily"];

                    out.tempC = current["temperature_2m"] | out.tempC;
                    out.humidity = current["relative_humidity_2m"] | out.humidity;
                    out.highC = daily["temperature_2m_max"][0] | out.highC;
                    out.lowC = daily["temperature_2m_min"][0] | out.lowC;

                    int code = current["weather_code"] | -1;
                    if (code >= 0) {
                        strncpy(out.condition, conditionForCode(code), sizeof(out.condition) - 1);
                        out.condition[sizeof(out.condition) - 1] = '\0';
                    }

                    out.valid = true;
                    ok = true;
                }
            }
            http.end();
        }
    }

    disconnectWiFi();
    return ok;
}

bool WeatherService::fetchLocationName(WeatherData &out) {
    bool ok = false;
    if (!connectWiFi()) {
        Serial.println("[Weather] WiFi connect failed/timed out (location).");
    } else {
        char url[192];
        snprintf(url, sizeof(url),
                 "https://api.bigdatacloud.net/data/reverse-geocode-client"
                 "?latitude=%.4f&longitude=%.4f&localityLanguage=en",
                 WEATHER_LATITUDE, WEATHER_LONGITUDE);

        WiFiClientSecure client;
        client.setInsecure(); // public read-only endpoint; skip cert pinning

        HTTPClient http;
        http.setTimeout(WEATHER_FETCH_TIMEOUT_MS);
        http.useHTTP10(true); // see fetch() -- avoids chunked-encoding vs ArduinoJson mismatch
        if (!http.begin(client, url)) {
            Serial.println("[Weather] HTTPClient::begin() failed (location).");
        } else {
            int status = http.GET();
            if (status != HTTP_CODE_OK) {
                Serial.printf("[Weather] Location HTTP GET failed, status=%d\n", status);
            } else {
                StaticJsonDocument<1536> doc;
                DeserializationError err = deserializeJson(doc, http.getStream());
                if (err) {
                    Serial.printf("[Weather] Location JSON parse failed: %s\n", err.c_str());
                } else {
                    // "locality" is the small-area name (e.g. a village/town);
                    // fall back to "city" if the point doesn't resolve one.
                    const char *name = doc["locality"] | (const char *)nullptr;
                    if (!name || name[0] == '\0') {
                        name = doc["city"] | (const char *)nullptr;
                    }
                    const char *subdivision = doc["principalSubdivision"] | (const char *)nullptr;

                    if (name && name[0] != '\0') {
                        if (subdivision && subdivision[0] != '\0') {
                            snprintf(out.location, sizeof(out.location), "%s, %s", name, subdivision);
                        } else {
                            strncpy(out.location, name, sizeof(out.location) - 1);
                            out.location[sizeof(out.location) - 1] = '\0';
                        }
                        ok = true;
                    }
                }
            }
            http.end();
        }
    }

    disconnectWiFi();
    return ok;
}
