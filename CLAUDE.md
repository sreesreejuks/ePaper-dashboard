# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Overview

A TRMNL-style split-panel dashboard for the LilyGo T5 4.7" ePaper S3 (ESP32-S3 with an E-Ink
display, PCF8563 RTC, WiFi). The panel is split into two widgets side by side: a **Clock** on the
left and **Weather** on the right, inside a shared dashed-border/grey-footer chrome. Time is kept
accurate via NTP sync over WiFi (with an RTC chip as the offline fallback), and weather is fetched
live from Open-Meteo for a fixed location, reverse-geocoded to a place name at boot.

Entry point: [epaper_dashboard.ino](epaper_dashboard.ino).

## Build / compile / upload

Same board settings as the rest of this Arduino sketch collection (see the parent folder's
`CLAUDE.md`) -- **ESP32S3 Dev Module**, 16MB flash, **OPI PSRAM required**, UART0/Hardware CDC
upload. Compile/upload from Arduino IDE, or `Sketch -> Export Compiled Binary` + `esptool` if
flashing manually (no `arduino-cli` installed in this environment as of writing).

**Before compiling**, copy `src/config/secrets.h.example` to `src/config/secrets.h` and fill in
your real WiFi SSID/password -- `secrets.h` is gitignored and `config.h` includes it directly.
Compilation fails without it.

## Dependencies

- ESP32 Arduino core (`WiFi.h`, `HTTPClient.h`, `WiFiClientSecure.h`, `esp_sntp.h`, etc.)
- **LilyGo-EPD47** board support library -- provides `epd_driver.h`, `utilities.h`, and the bundled
  bitmap fonts (`firasans.h`, `roboto12.h`, `roboto18.h`, `roboto32.h`). `Roboto32` is the largest
  font asset available; there is no larger one without generating a new font file.
- **SensorLib** (v0.19+) -- provides `SensorPCF8563.hpp` for the RTC chip.
- **ArduinoJson** (Benoit Blanchon) -- parses weather/geocoding API responses.

## Architecture

The active code path is a `Panel`-based split layout ("Milestone 4"). Two other things exist on
disk but are **not** part of this path and shouldn't be extended:
- `src/screens/clock_screen.*`, `weather_screen.*`, `system_screen.*`, `screen_manager.*` --
  dormant single-screen-at-a-time code from an earlier milestone, kept but unused.
- `src/screens/dashboard_chrome.*` -- superseded by the per-panel chrome drawn inline in
  `dashboard_layout.cpp`.

**Active components:**
- [`EpdDisplay`](src/display/epd_display.h) -- thin wrapper around the LilyGo `epd_driver` API;
  owns the framebuffer and the full-frame vs. partial-update drawing primitives.
- [`RtcClock`](src/rtc/rtc_clock.h) -- owns the PCF8563 chip. The chip's raw stored value is
  always UTC; `TIMEZONE_OFFSET_MINUTES` (config.h) is applied only when formatting for display
  (`timeString()`/`dateString()`/etc.), never written back to the chip. `syncFromNtp()` connects
  to WiFi, pulls UTC from NTP, writes it to the chip, then disconnects -- called once at boot and
  every `NTP_RESYNC_INTERVAL_MS` thereafter. If you ever reseed the chip from a local wall-clock
  time again (e.g. `__DATE__`/`__TIME__`), remember to subtract the timezone offset first, or the
  display will run `TIMEZONE_OFFSET_MINUTES` fast -- this exact bug shipped once already.
- [`WeatherService`](src/weather/weather_service.h) -- fetches live conditions from Open-Meteo
  (`fetch()`, every `WEATHER_UPDATE_INTERVAL_MS`) and a one-time reverse-geocoded location name
  from BigDataCloud (`fetchLocationName()`, once at boot only -- the dashboard doesn't move).
  Both open their own WiFi connection and close it when done, same pattern as `RtcClock`. Uses
  `http.useHTTP10(true)` deliberately: Open-Meteo's chunked-encoded response otherwise reaches
  ArduinoJson un-dechunked and fails to parse (`InvalidInput`).
- `ClockPanel` / `WeatherPanel` (`src/screens/`) -- implement the `Panel` interface
  (`drawContent()` for boot/full-refresh, `tick()` for partial updates). `WeatherPanel::tick()` is
  intentionally a no-op; weather only changes on the ~10-minute full-refresh cadence driven by
  `FULL_REFRESH_EVERY_N_UPDATES`, so a separate tick path isn't needed.
- [`DashboardLayout`](src/screens/dashboard_layout.h) -- composes the two panels' chrome (dashed
  border, grey footer with the panel's `title()`) and content areas; the only place that knows
  the actual pixel geometry of "left panel" vs. "right panel".
- [`config.h`](src/config/config.h) -- all tunable constants (timing intervals, layout geometry,
  weather coordinates). `secrets.h` is WiFi credentials only, kept separate so it can be
  gitignored.

**Refresh model:** `CLOCK_UPDATE_INTERVAL_MS` (60s) drives `loop()`. Every update calls
`DashboardLayout::tick()` (cheap partial redraw); every `FULL_REFRESH_EVERY_N_UPDATES`-th update
instead calls `fullClear()` + `DashboardLayout::drawFull()` (full flash-clear, resets e-paper
ghosting). NTP resync and weather fetch run on their own independent timers inside `loop()`,
outside this cadence -- they update in-memory data (`liveWeather`, the RTC chip) but don't force a
redraw themselves; the next scheduled tick/full-refresh just picks up whatever's current.
