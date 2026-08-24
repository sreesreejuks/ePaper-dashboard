#pragma once

// Wraps the onboard PCF8563 RTC chip (same one demo.ino talks to), exposing
// just the read/format operations the rest of the app needs.
#include <Wire.h>
#include <SensorPCF8563.hpp>

class RtcClock {
public:
    bool begin();
    bool isOnline() const { return online_; }

    // Connects to WiFi (config.h WIFI_SSID/WIFI_PASSWORD), fetches UTC via
    // NTP, and writes it to the RTC chip -- correcting whatever drift has
    // built up since the last sync. Blocks for up to NTP_SYNC_TIMEOUT_MS.
    // WiFi is always left disconnected+off afterward, whether or not the
    // sync succeeded. Returns true only if the RTC was actually updated.
    bool syncFromNtp();

    // NOTE: each method below formats into its own static buffer (see
    // rtc_clock.cpp) -- the pointer stays valid until the *same* method is
    // called again, but calling one does not invalidate another's buffer.
    const char *timeString();      // "HH:MM:SS"
    const char *dateString();      // "YYYY-MM-DD"
    const char *shortTimeString(); // "HH:MM" -- for a once-a-minute display
    const char *longDateString();  // "24 August 2026"

private:
    SensorPCF8563 rtc_;
    bool online_ = false;
};
