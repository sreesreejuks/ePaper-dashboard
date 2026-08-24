#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "src/config/config.h"
#include "src/display/epd_display.h"
#include "src/rtc/rtc_clock.h"
#include "src/weather/weather_data.h"
#include "src/weather/weather_service.h"
#include "src/screens/panel.h"
#include "src/screens/clock_panel.h"
#include "src/screens/weather_panel.h"
#include "src/screens/dashboard_layout.h"

// ---- Power model ----
// There is no continuously-running loop() driving updates anymore: every
// wake is one full run of setup() -- render, sleep -- and deep sleep
// restarts execution from setup() again when the timer fires. This is what
// lets the ESP32-S3 draw near-zero current between refreshes instead of
// staying awake polling millis() timers.
//
// The wake interval (1 minute, see DEEP_SLEEP_INTERVAL_SEC) is driven by the
// clock, which must show the current minute -- it does NOT mean WiFi/NTP/
// weather happen every wake too. Those are independently gated by
// NTP_SYNC_EVERY_N_WAKES / WEATHER_REFRESH_EVERY_N_WAKES (config.h) using
// the wakeCount below, so most wakes touch only the RTC chip (no WiFi at
// all) and repaint the clock digits from cached/last-known data.
//
// Ordinary globals (liveWeather, etc.) are wiped every wake along with the
// rest of RAM. Only RTC_DATA_ATTR variables below survive deep sleep (they
// live in RTC slow memory, which stays powered) -- that's how the last
// known-good weather, the one-time location lookup, and the refresh
// cadences persist across wakes.
RTC_DATA_ATTR WeatherData savedWeather = {0.0f, "", 0.0f, 0.0f, 0, "", false};
RTC_DATA_ATTR bool locationResolved = false;
RTC_DATA_ATTR uint32_t wakeCount = 0; // counts wakes; drives NTP/weather/full-refresh cadences

EpdDisplay display;
RtcClock rtcClock;
WeatherService weatherService;
WeatherData liveWeather = {0.0f, "", 0.0f, 0.0f, 0, "", false};

ClockPanel clockPanel(rtcClock);
WeatherPanel weatherPanel(liveWeather);

namespace {

bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
    }
    return WiFi.status() == WL_CONNECTED;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void goToSleep() {
    Serial.flush();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US);
    esp_deep_sleep_start(); // never returns -- execution resumes at setup() on the next wake
}

} // namespace

void setup() {
    Serial.begin(115200);
    delay(200);

    // Carry the last known-good weather forward from before this wake, so
    // that a wake which doesn't fetch (or whose fetch fails) still has
    // something real to show rather than falling back to "Weather unavailable".
    liveWeather = savedWeather;
    if (liveWeather.location[0] == '\0') {
        strncpy(liveWeather.location, WEATHER_LOCATION_LABEL, sizeof(liveWeather.location) - 1);
        liveWeather.location[sizeof(liveWeather.location) - 1] = '\0';
    }

    rtcClock.begin(); // I2C RTC chip -- no WiFi needed; provides the current time every wake, synced or not

    // wakeCount == 0 (first-ever boot) satisfies every modulo check below,
    // so first boot always syncs NTP, fetches weather, and does the one-time
    // location lookup with no special-casing required.
    bool ntpDue = (wakeCount % NTP_SYNC_EVERY_N_WAKES == 0);
    bool weatherDue = (wakeCount % WEATHER_REFRESH_EVERY_N_WAKES == 0);
    bool needsWifi = ntpDue || weatherDue || !locationResolved;

    if (!needsWifi) {
        Serial.println("Nothing due this wake -- skipping WiFi, rendering from cached RTC time/weather.");
    } else if (!connectWiFi()) {
        Serial.println("WiFi connect failed/timed out -- using RTC time / cached weather this wake.");
        disconnectWiFi(); // WiFi.begin() was still called above; make sure the radio is off before sleeping
    } else {
        Serial.println("WiFi connected.");

        if (ntpDue) {
            if (rtcClock.syncFromNtp()) {
                Serial.println("RTC synced from NTP.");
            } else {
                Serial.println("NTP sync failed -- using RTC's own time.");
            }
        }

        if (!locationResolved) {
            if (weatherService.fetchLocationName(liveWeather)) {
                locationResolved = true;
                strncpy(savedWeather.location, liveWeather.location, sizeof(savedWeather.location) - 1);
                savedWeather.location[sizeof(savedWeather.location) - 1] = '\0';
                Serial.printf("Location resolved: %s\n", liveWeather.location);
            } else {
                Serial.println("Location fetch failed -- will retry next wake.");
            }
        }

        if (weatherDue) {
            if (weatherService.fetch(liveWeather)) {
                Serial.println("Weather fetched.");
                savedWeather = liveWeather; // persist across the next deep sleep
            } else {
                Serial.println("Weather fetch failed -- keeping last-known-good weather.");
            }
        }

        disconnectWiFi();
    }

    // Render every wake, unconditionally: the clock must show the current
    // minute regardless of whether WiFi/NTP/weather happened this wake,
    // since RTC time is independent of connectivity. Full flash-clear
    // (ghost-reset) only every FULL_REFRESH_EVERY_N_WAKES renders; every
    // other render is a cheap partial tick that updates just the clock
    // digits without flashing the whole panel.
    if (!display.begin()) {
        Serial.println("Display framebuffer allocation failed (PSRAM?) - halting.");
        while (true) delay(1000);
    }
    display.powerOn();

    bool fullRefresh = (wakeCount % FULL_REFRESH_EVERY_N_WAKES == 0);
    if (fullRefresh) {
        display.fullClear();
        DashboardLayout::drawFull(display, clockPanel, weatherPanel);
    } else {
        DashboardLayout::tick(display, clockPanel, weatherPanel);
    }
    wakeCount++;

    // powerOffAll() (not powerOff()) -- also cuts POWER_EN and the status
    // LED, not just the panel, since we're about to sit in deep sleep for
    // DEEP_SLEEP_INTERVAL_SEC rather than looping straight back around.
    display.powerOffAll();

    goToSleep();
}

void loop() {
    // Never reached: setup() always ends by entering deep sleep, which
    // restarts execution from setup() on the next wake.
}
