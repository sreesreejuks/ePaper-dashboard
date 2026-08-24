#pragma once

// Pin/board definitions (BOARD_SDA, BOARD_SCL, EPD_WIDTH, EPD_HEIGHT, etc.)
// come from the LilyGo-EPD47 library's own utilities.h / epd_driver.h --
// never redefine hardware pins here, only app-level layout/timing constants.
#include "utilities.h"
#include "epd_driver.h" // EPD_WIDTH / EPD_HEIGHT -- needed regardless of include order

// ---- timezone ----
// Applied by RtcClock when formatting the RTC's raw reading for display --
// the RTC chip itself is never adjusted, it just keeps ticking whatever
// wall-clock time it was last set to. Default is IST (Asia/Kolkata,
// UTC+5:30, no DST). Phase 2's NTP sync will use this same constant to
// convert UTC to local time, so this isn't throwaway.
constexpr int TIMEZONE_OFFSET_MINUTES = 330; // UTC+5:30

// ---- WiFi / NTP ----
// RtcClock::syncFromNtp() uses these to correct the RTC's drift. Both it and
// WeatherService assume the caller (epaper_dashboard.ino) already brought
// WiFi up via connectWiFi() -- they no longer manage their own WiFi
// connect/disconnect cycle, so one wake only pays for one radio session
// covering NTP + location + weather instead of three. WIFI_SSID/
// WIFI_PASSWORD come from secrets.h (gitignored, not committed) -- copy
// secrets.h.example to secrets.h and fill in your real credentials.
#include "secrets.h"
constexpr const char *NTP_SERVER_1 = "pool.ntp.org";
constexpr const char *NTP_SERVER_2 = "time.nist.gov";
constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 15UL * 1000UL; // give up and keep ticking on RTC alone
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15UL * 1000UL; // per-wake WiFi connect attempt before giving up

// ---- weather (live, via Open-Meteo -- free, no API key required) ----
// Coordinates for Kottayam town, Kerala (nearest place Open-Meteo's geocoder
// resolves to Veloor/PIN 686003 -- a few km off, which doesn't matter at
// weather-forecast resolution). Adjust if you want a different point.
constexpr double WEATHER_LATITUDE = 9.587;
constexpr double WEATHER_LONGITUDE = 76.521;
// Fallback shown only if WeatherService::fetchLocationName()'s one-time
// reverse geocode fails (e.g. no WiFi at boot) -- normally the panel shows
// whatever that lookup resolves the coordinates above to.
constexpr const char *WEATHER_LOCATION_LABEL = "Veloor, Kottayam";
constexpr uint32_t WEATHER_FETCH_TIMEOUT_MS = 15UL * 1000UL;

// ---- timing ----
constexpr uint32_t TICK_INTERVAL_MS = 1000;
constexpr uint32_t FULL_REFRESH_INTERVAL_MS = 15UL * 60UL * 1000UL;   // ghost-reset backstop
constexpr uint32_t SCREEN_ROTATION_INTERVAL_MS = 15UL * 1000UL;       // CLOCK -> WEATHER -> SYSTEM -> CLOCK

// ---- chrome layout (960x540 panel): header bar, page dots, footer rule ----
constexpr int32_t CONTENT_MARGIN_X = 30;
constexpr int32_t HEADER_TITLE_X = 30;
constexpr int32_t HEADER_TITLE_Y = 50;
constexpr int32_t HEADER_RULE_Y = 72;
constexpr int32_t PAGE_DOT_Y = 35;
constexpr int32_t PAGE_DOT_RADIUS = 7;
constexpr int32_t PAGE_DOT_SPACING = 26;
constexpr int32_t FOOTER_RULE_Y = 500;
constexpr int32_t FOOTER_TEXT_Y = 524;

// ---- clock screen ----
constexpr int32_t TIME_CURSOR_X = 260;
constexpr int32_t TIME_CURSOR_Y = 280;
constexpr int32_t TIME_AREA_X = 230;
constexpr int32_t TIME_AREA_Y = 190;
constexpr int32_t TIME_AREA_W = 520;
constexpr int32_t TIME_AREA_H = 120;

constexpr int32_t DATE_CURSOR_X = 260;
constexpr int32_t DATE_CURSOR_Y = 380;
constexpr int32_t DATE_AREA_X = 230;
constexpr int32_t DATE_AREA_Y = 335;
constexpr int32_t DATE_AREA_W = 520;
constexpr int32_t DATE_AREA_H = 70;

// ---- weather screen (mock data in Milestone 2) ----
constexpr int32_t WEATHER_TEMP_X = 60;
constexpr int32_t WEATHER_TEMP_Y = 250;
constexpr int32_t WEATHER_COND_X = 60;
constexpr int32_t WEATHER_COND_Y = 300;
constexpr int32_t WEATHER_RANGE_X = 60;
constexpr int32_t WEATHER_RANGE_Y = 355;
constexpr int32_t WEATHER_HUMIDITY_X = 60;
constexpr int32_t WEATHER_HUMIDITY_Y = 400;

// ---- system screen ----
constexpr int32_t SYSTEM_LINE_X = 60;
constexpr int32_t SYS_LINE1_Y = 150; // chip
constexpr int32_t SYS_LINE2_Y = 195; // cpu / flash
constexpr int32_t SYS_LINE3_Y = 240; // free heap
constexpr int32_t SYS_LINE4_Y = 285; // free psram
constexpr int32_t SYS_LINE5_Y = 330; // rtc status
constexpr int32_t SYS_LINE6_Y = 385; // uptime (ticks every second)
constexpr int32_t SYS_UPTIME_AREA_X = 50;
constexpr int32_t SYS_UPTIME_AREA_Y = 335;
constexpr int32_t SYS_UPTIME_AREA_W = 500;
constexpr int32_t SYS_UPTIME_AREA_H = 65;

// ---- simplified centered test screen (title + clock + date only) ----
// Dormant Milestone-2 screens (ClockScreen/WeatherScreen/SystemScreen/
// ScreenManager) are left on disk untouched but unused for now -- this is a
// focused pass to get core rendering ghost-free before resuming the dashboard.
constexpr int32_t TEST_TITLE_Y = 180;  // "LILYGO T5 ePaper S3"
constexpr int32_t TEST_TIME_Y = 320;   // "13:42"
constexpr int32_t TEST_DATE_Y = 400;   // "24 August 2026"

// Single combined region covering BOTH the time and date lines together --
// whitewashed as one unit before every per-minute redraw, generous enough to
// contain any real date string (longest month names) with margin, so a
// month-name-length change can never leave a leftover fragment outside it.
constexpr int32_t TEST_DYNAMIC_AREA_X = 80;
constexpr int32_t TEST_DYNAMIC_AREA_Y = 230;
constexpr int32_t TEST_DYNAMIC_AREA_W = 800;
constexpr int32_t TEST_DYNAMIC_AREA_H = 210;

// ---- deep sleep (power optimization) ----
// The whole board wakes, renders, then sleeps -- there is no continuously-
// running loop() anymore. The wake interval is 1 minute so the clock/date
// (read straight from the RTC, independent of WiFi) stays current; NTP sync
// and weather fetches are much rarer and gated separately below, NOT tied to
// this constant -- see NTP_SYNC_EVERY_N_WAKES / WEATHER_REFRESH_EVERY_N_WAKES.
constexpr uint64_t DEEP_SLEEP_INTERVAL_SEC = 1UL * 60UL; // 1 minute
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = DEEP_SLEEP_INTERVAL_SEC * 1000000ULL;

// Data-refresh cadences, expressed as "every Nth wake" (wakeCount % N == 0)
// rather than a wall-clock duration. This is accurate without separately
// tracking last-synced timestamps: deep-sleep timer wakeups are driven by
// the RTC's own hardware timer and don't meaningfully drift at this
// timescale, so a wake count is a reliable proxy for elapsed time.
// wakeCount == 0 (first-ever boot) satisfies every modulo check below
// automatically (0 % N == 0), so first boot always does everything with no
// special-casing needed.
constexpr uint32_t NTP_SYNC_EVERY_N_WAKES = 360;       // 6 hours at the 1-minute wake interval above
constexpr uint32_t WEATHER_REFRESH_EVERY_N_WAKES = 30; // 30 minutes at the 1-minute wake interval above

// A full flash-clear (ghost-reset) happens only every Nth render; the rest
// are cheap partial ticks (just the clock digits). Scaled to preserve the
// same ~1-hour wall-clock cadence as before (was 12 renders at a 5-minute
// interval) now that wakes are 5x more frequent -- changing the wake
// interval without rescaling this would have silently turned ghost-resets
// into a ~12-minute cadence instead of ~hourly.
constexpr uint16_t FULL_REFRESH_EVERY_N_WAKES = 60;

// ---- Milestone 4: TRMNL-style split-panel dashboard (2 panels side by side) ----
constexpr int32_t OUTER_MARGIN = 20;
constexpr int32_t PANEL_GAP = 20;    // horizontal gap between the two panels
constexpr int32_t PANEL_Y = 20;
constexpr int32_t PANEL_H = 500;
constexpr int32_t PANEL_W = (EPD_WIDTH - 2 * OUTER_MARGIN - PANEL_GAP) / 2;
constexpr int32_t PANEL_LEFT_X = OUTER_MARGIN;
constexpr int32_t PANEL_RIGHT_X = PANEL_LEFT_X + PANEL_W + PANEL_GAP;
constexpr int32_t PANEL_PADDING = 16; // inner padding between a panel's border and its content
constexpr int32_t FOOTER_H = 40;      // footer strip height at the bottom of each panel
constexpr uint8_t FOOTER_GRAY_LEVEL = 210; // 0=black .. 255=white
constexpr int32_t DASH_LEN = 6;       // dashed-border segment length
constexpr int32_t GAP_LEN = 5;        // dashed-border gap length
