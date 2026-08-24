# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Overview

A TRMNL-style split-panel dashboard for the LilyGo T5 4.7" ePaper S3 (ESP32-S3 with an E-Ink
display, PCF8563 RTC, WiFi). The panel is split into two widgets side by side: a **Clock** on the
left and **Weather** on the right, inside a shared dashed-border/grey-footer chrome. Time is kept
accurate via NTP sync over WiFi (with an RTC chip as the offline fallback), and weather is fetched
live from Open-Meteo for a fixed location, reverse-geocoded to a place name once.

The board runs as a **wake -> update -> deep sleep** cycle, not a continuously-running loop --
see "Refresh model" below. This is deliberate for battery life: the e-paper panel holds its last
image with zero power draw while the ESP32-S3 is in deep sleep, so there's no benefit to keeping
the CPU awake between updates.

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
  owns the framebuffer and the full-frame vs. partial-update drawing primitives. `powerOff()` vs
  `powerOffAll()` matters: the latter also cuts `POWER_EN` and the status LED (not just the
  panel), so it's what's used right before deep sleep; `powerOff()` is for when execution is about
  to keep running (not applicable currently, since every wake ends in deep sleep, but kept for any
  future non-sleep caller).
- [`RtcClock`](src/rtc/rtc_clock.h) -- owns the PCF8563 chip. The chip's raw stored value is
  always UTC; `TIMEZONE_OFFSET_MINUTES` (config.h) is applied only when formatting for display
  (`timeString()`/`dateString()`/etc.), never written back to the chip. `syncFromNtp()` pulls UTC
  from NTP and writes it to the chip, but does **not** manage WiFi itself -- it assumes the caller
  already connected (see "Refresh model" below), and is only called when NTP sync is actually due
  (`NTP_SYNC_EVERY_N_WAKES`), not every wake. If you ever reseed the chip from a local wall-clock
  time again (e.g. `__DATE__`/`__TIME__`), remember to subtract the timezone offset first, or the
  display will run `TIMEZONE_OFFSET_MINUTES` fast -- this exact bug shipped once already.
- [`WeatherService`](src/weather/weather_service.h) -- fetches live conditions from Open-Meteo
  (`fetch()`, only called when due -- see `WEATHER_REFRESH_EVERY_N_WAKES`) and a one-time
  reverse-geocoded location name from BigDataCloud (`fetchLocationName()`, retried on future wakes
  until it succeeds once -- the dashboard doesn't move, so after that it's never called again).
  Like `RtcClock`, assumes the caller already has WiFi connected -- it does not open/close its own
  connection. Uses `http.useHTTP10(true)` deliberately: Open-Meteo's chunked-encoded response
  otherwise reaches ArduinoJson un-dechunked and fails to parse (`InvalidInput`).
- `ClockPanel` / `WeatherPanel` (`src/screens/`) -- implement the `Panel` interface
  (`drawContent()` for boot/full-refresh, `tick()` for partial updates). `WeatherPanel::tick()` is
  intentionally a no-op; weather only changes on the full-refresh cadence driven by
  `FULL_REFRESH_EVERY_N_WAKES`, so a separate tick path isn't needed.
- [`DashboardLayout`](src/screens/dashboard_layout.h) -- composes the two panels' chrome (dashed
  border, grey footer with the panel's `title()`) and content areas; the only place that knows
  the actual pixel geometry of "left panel" vs. "right panel".
- [`config.h`](src/config/config.h) -- all tunable constants (timing intervals, layout geometry,
  weather coordinates). `secrets.h` is WiFi credentials only, kept separate so it can be
  gitignored.

**Refresh model:** there is no `loop()` -- each deep-sleep wake runs `setup()` once end to end and
deep sleep restarts execution at `setup()` on the next wake. `DEEP_SLEEP_INTERVAL_SEC` (1 minute)
is deliberately short because the clock/date must show the current minute, read straight from the
RTC chip -- **this does not mean WiFi/NTP/weather happen every wake too.** Those are gated
independently against `wakeCount` (an `RTC_DATA_ATTR` global, survives deep sleep):
`ntpDue = wakeCount % NTP_SYNC_EVERY_N_WAKES == 0` (6h) and
`weatherDue = wakeCount % WEATHER_REFRESH_EVERY_N_WAKES == 0` (30min). WiFi is only brought up at
all if `ntpDue || weatherDue || !locationResolved` -- most wakes touch only the RTC chip (no radio
at all) and repaint the clock from cached/last-known data. `wakeCount == 0` (first boot) satisfies
every modulo check automatically, so first boot always does everything with no special-casing.
This wake-count approach works because deep-sleep timer wakeups are driven by the RTC hardware
timer and don't meaningfully drift at this timescale, so wake count is an accurate stand-in for
elapsed time without needing to track timestamps separately.

Only `RTC_DATA_ATTR` globals (`savedWeather`, `locationResolved`, `wakeCount` in
`epaper_dashboard.ino`) survive a wake, since deep sleep wipes ordinary RAM; everything else,
including the `EpdDisplay`/`RtcClock`/`WeatherService`/`Panel` objects, is freshly constructed
every wake. `wakeCount` also drives the full-vs-partial render policy: `DashboardLayout::tick()`
(cheap partial redraw, just the clock digits) every wake, except every
`FULL_REFRESH_EVERY_N_WAKES`-th (60, ~hourly), which does `fullClear()` +
`DashboardLayout::drawFull()` instead (full flash-clear, resets e-paper ghosting -- also the only
point at which freshly-fetched weather actually gets painted, since `WeatherPanel::tick()` is a
no-op). Render happens **every** wake unconditionally, regardless of whether WiFi was needed or
connected that wake -- only `EpdDisplay::begin()` failing (PSRAM alloc) halts instead of sleeping.

**WiFi failure handling:** if `connectWiFi()` is attempted (because something was due) and fails,
that wake's NTP sync / weather fetch / location lookup are simply skipped and retried on a future
wake -- the render still happens using cached/last-known data (`savedWeather`, the RTC's own
ticking time), never blocked on WiFi succeeding.
