#include <Arduino.h>

#include "src/config/config.h"
#include "src/display/epd_display.h"
#include "src/rtc/rtc_clock.h"
#include "src/weather/weather_data.h"
#include "src/weather/weather_service.h"
#include "src/screens/panel.h"
#include "src/screens/clock_panel.h"
#include "src/screens/weather_panel.h"
#include "src/screens/dashboard_layout.h"

EpdDisplay display;
RtcClock rtcClock;
WeatherService weatherService;

// Phase 3: live data from WeatherService, fetched over WiFi (Open-Meteo).
// Starts invalid -- WeatherPanel shows "Weather unavailable" until the first
// fetch succeeds. WeatherPanel's code does not change, it just reads
// whatever's in this struct. location starts empty and is filled once at
// boot (see setup()), falling back to config.h's WEATHER_LOCATION_LABEL if
// the reverse-geocode fetch fails.
WeatherData liveWeather = {0.0f, "", 0.0f, 0.0f, 0, "", false};

ClockPanel clockPanel(rtcClock);
WeatherPanel weatherPanel(liveWeather);

uint32_t nextUpdate = 0;
uint32_t nextNtpSync = 0;
uint32_t nextWeatherFetch = 0;
uint16_t updatesSinceFullRefresh = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!display.begin()) {
        Serial.println("Display framebuffer allocation failed (PSRAM?) - halting.");
        while (true) delay(1000);
    }
    display.powerOn();
    display.fullClear();

    rtcClock.begin();
    if (rtcClock.syncFromNtp()) {
        Serial.println("RTC synced from NTP.");
    } else {
        Serial.println("NTP sync failed/unavailable -- using RTC's own time.");
    }
    strncpy(liveWeather.location, WEATHER_LOCATION_LABEL, sizeof(liveWeather.location) - 1);
    liveWeather.location[sizeof(liveWeather.location) - 1] = '\0';
    if (weatherService.fetchLocationName(liveWeather)) {
        Serial.printf("Location resolved: %s\n", liveWeather.location);
    } else {
        Serial.println("Location fetch failed -- using fallback label.");
    }
    if (weatherService.fetch(liveWeather)) {
        Serial.println("Weather fetched.");
    } else {
        Serial.println("Weather fetch failed -- showing unavailable until next retry.");
    }
    DashboardLayout::drawFull(display, clockPanel, weatherPanel);

    display.powerOff();

    nextUpdate = millis() + CLOCK_UPDATE_INTERVAL_MS;
    nextNtpSync = millis() + NTP_RESYNC_INTERVAL_MS;
    nextWeatherFetch = millis() + WEATHER_UPDATE_INTERVAL_MS;
    updatesSinceFullRefresh = 0;
}

void loop() {
    if (millis() >= nextNtpSync) {
        nextNtpSync = millis() + NTP_RESYNC_INTERVAL_MS;
        rtcClock.syncFromNtp();
    }

    if (millis() >= nextWeatherFetch) {
        nextWeatherFetch = millis() + WEATHER_UPDATE_INTERVAL_MS;
        weatherService.fetch(liveWeather); // updates liveWeather in place; next full refresh picks it up
    }

    if (millis() < nextUpdate) {
        return;
    }
    nextUpdate = millis() + CLOCK_UPDATE_INTERVAL_MS;
    updatesSinceFullRefresh++;

    display.powerOn();

    if (updatesSinceFullRefresh >= FULL_REFRESH_EVERY_N_UPDATES) {
        display.fullClear();
        DashboardLayout::drawFull(display, clockPanel, weatherPanel);
        updatesSinceFullRefresh = 0;
    } else {
        DashboardLayout::tick(display, clockPanel, weatherPanel);
    }

    display.powerOff();
}
