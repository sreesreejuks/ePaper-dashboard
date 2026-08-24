# Changelog

Summary of work completed on the ePaper dashboard (`epaper_dashboard.ino`) in this session.

## 1. Fixed wrong clock time (double timezone offset)

**Problem:** The display showed a time several hours ahead of actual IST (e.g. 20:21 shown when it was really 12:19). `RtcClock::begin()` seeded the RTC chip from the compiling PC's local clock (`__DATE__`/`__TIME__`, already IST), but every read (`timeString()`, `shortTimeString()`, etc.) also added `TIMEZONE_OFFSET_MINUTES` (+5:30) on top — double-applying the IST offset.

**Fix:** `src/rtc/rtc_clock.cpp` now subtracts the timezone offset before writing the compile-time value into the chip, so the RTC's raw value stays in UTC (matching what the read-side code already assumes). Also did a one-time forced reseed to correct the time that was already wrong in the chip.

## 2. Added NTP time sync over WiFi

**Problem:** There was no way to correct RTC drift or get exact time without manually recompiling; the project's own comments flagged this as a planned "Phase 2."

**Fix:** Added `RtcClock::syncFromNtp()` (`src/rtc/rtc_clock.h/.cpp`) — connects to WiFi, pulls UTC from `pool.ntp.org`/`time.nist.gov`, writes it to the RTC chip, then disconnects WiFi. Called once at boot and every 6 hours thereafter (`NTP_RESYNC_INTERVAL_MS` in `config.h`) to correct drift. Falls back silently to the RTC's own ticking time if WiFi/NTP is unavailable.

## 3. Added live weather data (replacing the hardcoded mock)

**Problem:** Weather was a hardcoded `WeatherData mockWeather` struct with static values.

**Fix:** Added `WeatherService` (`src/weather/weather_service.h/.cpp`) which fetches live conditions from Open-Meteo (free, no API key) for Kottayam/Veloor's coordinates, fills the same `WeatherData` struct the panel already reads, and re-fetches every 30 minutes (`WEATHER_UPDATE_INTERVAL_MS`). `WeatherPanel`'s rendering code needed no changes for this part since it only ever reads the struct.

Bugs fixed along the way:
- JSON parse buffer (`StaticJsonDocument`) was sized too small (768 bytes) for the real response and silently failed — increased to 2048 bytes.
- Open-Meteo's chunked transfer-encoded response was being handed to ArduinoJson raw (un-dechunked), causing `InvalidInput` parse errors — fixed by forcing HTTP/1.0 (`http.useHTTP10(true)`), which gets a plain `Content-Length` body instead.

## 4. Added dynamic location name (reverse geocoding)

**Problem:** The weather panel's location label was a hardcoded string (`"Veloor, Kottayam"`).

**Fix:** Added `WeatherService::fetchLocationName()`, which reverse-geocodes `WEATHER_LATITUDE`/`WEATHER_LONGITUDE` via BigDataCloud's free, no-API-key endpoint once at boot (the dashboard doesn't move, so this only needs to run once, unlike the weather data itself). Falls back to the hardcoded `WEATHER_LOCATION_LABEL` in `config.h` if the lookup fails.

## 5. UI polish pass (TRMNL-style monochrome look)

Kept the existing 960x540 split-panel layout, dotted borders, grey footer labels, and partial-refresh behavior unchanged; only the content composition inside each panel changed:

- **Clock panel** (`clock_panel.cpp`): re-tuned vertical layout so the time+date pair centers as a block within the widget; time is now drawn with a 1px-offset double-draw (faux-bold) for more visual weight, since `Roboto32` is already the largest bitmap font bundled with the board's library (a true size increase would need generating a new font asset).
- **Weather panel** (`weather_panel.cpp`): replaced the old two-column layout (temperature | condition, each squeezed into half the panel width) with a single centered column — icon, then temperature (kept prominent), then condition text. Condition text now measures its own width and drops from Roboto18 to Roboto12 if it would run past the panel's edge, which is what fixes long strings like "Thunderstorm w/ Hail" from clipping. Dropped the small "Temperature"/"Conditions" captions for a cleaner centered composition (data shown is unchanged, only those labels were removed).

## Known trade-offs / things left as-is

- Weather source is Open-Meteo (free, model-based forecast), not a live station feed — so it can read a couple degrees off from Google/TRMNL's station-based sources. Switching to a station-based API (e.g. WeatherAPI.com) was discussed but not done; would need a free-tier signup + API key.
- `WEATHER_LATITUDE`/`WEATHER_LONGITUDE` in `config.h` point at Kottayam town center (Open-Meteo's geocoder doesn't resolve "Veloor" specifically) — a few km off from the exact PIN 686003 location, which doesn't matter at weather-forecast resolution.
