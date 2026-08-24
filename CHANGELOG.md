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

## 6. Deep-sleep power optimization (wake -> update -> sleep)

**Problem:** The board ran a continuously-awake `loop()` driven by `millis()` timers (clock tick
every minute, NTP resync every 6h, weather fetch every 30min, full refresh every ~10min) --
keeping the ESP32-S3 powered and spinning the whole time between updates, which wastes battery on
a device whose display (e-paper) needs zero power to hold its last image.

**Fix:** Replaced the `loop()`-driven model with a single wake -> connect -> render -> sleep cycle
in `setup()` (`epaper_dashboard.ino`); `loop()` is now unreachable. Each wake: connects WiFi once
(`connectWiFi()`), syncs the RTC from NTP, does the one-time location lookup (retried until it
succeeds), fetches weather, disconnects WiFi, renders (partial tick, or a full flash-clear every
`FULL_REFRESH_EVERY_N_WAKES` renders to reset e-paper ghosting), calls `EpdDisplay::powerOffAll()`
(new -- also cuts `POWER_EN`/the status LED, not just the panel), then calls
`esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US)` + `esp_deep_sleep_start()`. Waking from
deep sleep restarts execution at `setup()`, so `RTC_DATA_ATTR` globals in `epaper_dashboard.ino`
(`savedWeather`, `locationResolved`, `renderCount`) are what persist state across wakes -- ordinary
RAM, including the previous wake's `liveWeather`, is wiped every time.

`RtcClock::syncFromNtp()` and `WeatherService::fetch()`/`fetchLocationName()` no longer open/close
their own WiFi connection -- they now assume the caller already connected, so one wake pays for
one WiFi session instead of up to three. If WiFi fails to connect at all (and it isn't the very
first boot), the wake skips the display entirely and goes straight back to sleep, leaving the
panel showing whatever it last rendered. If WiFi connects but only the weather fetch fails, the
render still happens using the last known-good weather (`savedWeather`) instead of showing
"Weather unavailable". `DEEP_SLEEP_INTERVAL_SEC` (`config.h`, default 5 minutes) is the single
constant that controls the wake interval.

## 6b. Corrected the deep-sleep power-management architecture

**Problem:** Section 6's deep-sleep refactor made every wake connect WiFi and re-sync NTP *and* re-fetch weather, with no interval gating at all -- at the then-5-minute wake interval this was already wasteful, and would have gotten 5x worse un-gated when the wake interval was shortened to 1 minute for the clock. `FULL_REFRESH_EVERY_N_WAKES` was also tuned in wake-count terms (12, ≈1 hour at 5-minute wakes) and would have silently become a ~12-minute cadence once wakes moved to 1 minute. There was also a "skip rendering entirely if WiFi fails" branch that conflicts with the clock needing to update every minute regardless of connectivity.

**Fix:** Added `wakeCount` (`RTC_DATA_ATTR`, survives deep sleep) and gated NTP sync / weather fetch as "every Nth wake" (`NTP_SYNC_EVERY_N_WAKES = 360` → 6h, `WEATHER_REFRESH_EVERY_N_WAKES = 30` → 30min, both at the 1-minute wake interval) instead of every wake. WiFi is now only brought up at all when something is actually due (`ntpDue || weatherDue || !locationResolved`); most wakes touch only the RTC chip. `wakeCount == 0` (first boot) satisfies every modulo check automatically. Scaled `FULL_REFRESH_EVERY_N_WAKES` from 12 to 60 to preserve the original ~1-hour ghost-reset cadence now that wakes are 5x more frequent. Removed the WiFi-failure skip-render branch -- the dashboard now renders (at least a partial clock-digit tick) every wake unconditionally, since RTC time is independent of WiFi. `DEEP_SLEEP_INTERVAL_SEC` is now 1 minute.

## 7. Published the repo to GitHub, with a secrets fix along the way

**Problem:** The project wasn't in version control with a remote yet, and `src/config/config.h` had the real WiFi password hardcoded in plaintext — committing it as-is would have pushed that password straight to GitHub.

**Fix:** Split WiFi credentials into `src/config/secrets.h` (gitignored, never committed) with `secrets.h.example` as the committed template; `config.h` now just `#include`s it. Added a `.gitignore` (excluding `secrets.h` and the 37MB `build/` output folder). Committed everything and pushed directly to `master` on `github.com/sreesreejuks/ePaper-dashboard` — no PR needed since it was the very first commit with nothing to diff against.

## 8. Added project documentation

**Fix:** Added `CLAUDE.md` (architecture/build guidance for future Claude Code sessions — active vs. dormant code paths, the RTC double-offset gotcha, the refresh model) and `README.md` (GitHub-facing: features, hardware/Arduino IDE setup, library list, `secrets.h` setup steps, project layout), plus a photo of the running dashboard in `public/image.jpg`, referenced from the README.

## 9. Fixed a LilyGo-EPD47 library compile error (IDF5 RMT driver)

**Problem:** Compiling failed inside the vendored `LilyGo-EPD47` library (not this repo's own code) with cascading "unknown type name" errors in `rmt_pulse.c` (`rmt_channel_handle_t`, `rmt_tx_channel_config_t`, etc.). That file had been manually patched at some point to target ESP-IDF 5's new RMT driver, but the patch didn't compile against the installed ESP32 core (3.3.11, IDF 5.5). Initially recommended downgrading the ESP32 core to a pre-IDF5 version, but found that other files in the same library (`ed047tc1.c`, `epd_driver.c`, `i2s_data_bus.c`, `libjpeg.c`) had their own deliberate, working IDF5 patches — including a documented fix for a real panic ("required in IDF 5.x; omitting causes 'unknown clock source 0' panic") — so downgrading would have thrown away real, already-working fixes.

**Fix:** Rewrote `rmt_pulse.c`'s IDF5 branch instead, verifying every struct field name and function signature (`rmt_tx_channel_config_t`, `rmt_copy_encoder_config_t`, `rmt_transmit_config_t`, `rmt_symbol_word_t`, `rmt_new_tx_channel`, etc.) directly against the headers installed under the ESP32 core's `esp_driver_rmt` package, and added explicit `#include <esp_idf_version.h>` / `#include <driver/rmt_types.h>` as a safety net. Outcome not yet confirmed — awaiting a fresh compile attempt.

## Known trade-offs / things left as-is

- Weather source is Open-Meteo (free, model-based forecast), not a live station feed — so it can read a couple degrees off from Google/TRMNL's station-based sources. Switching to a station-based API (e.g. WeatherAPI.com) was discussed but not done; would need a free-tier signup + API key.
- `WEATHER_LATITUDE`/`WEATHER_LONGITUDE` in `config.h` point at Kottayam town center (Open-Meteo's geocoder doesn't resolve "Veloor" specifically) — a few km off from the exact PIN 686003 location, which doesn't matter at weather-forecast resolution.
- Renaming the project folder from `epaper_dashboard` to `ePaper-dashboard` (to match the GitHub repo name) is still pending — blocked by the folder being locked/in-use (likely by the IDE having it open as a workspace). Renaming will also give this project a fresh, empty Claude Code memory folder going forward, since that's keyed by the absolute path.
