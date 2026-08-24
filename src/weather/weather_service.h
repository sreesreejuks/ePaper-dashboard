#pragma once

#include "weather_data.h"

// Fetches live conditions from Open-Meteo (api.open-meteo.com) -- free, no
// API key required. Assumes the caller has already brought WiFi up (e.g.
// via connectWiFi() in ePaper-dashboard.ino) and leaves it connected
// afterward -- this class does not manage the WiFi connection itself, so a
// single WiFi session can cover NTP sync, location lookup, and both fetch()
// calls in one wake instead of paying for a separate connect each time.
class WeatherService {
public:
    // On success, fills `out` (setting out.valid = true) and returns true.
    // On any failure (WiFi not connected, HTTP, parse), leaves `out`
    // untouched and returns false -- callers just keep showing the
    // last-known-good data.
    bool fetch(WeatherData &out);

    // One-time reverse geocode of config.h's WEATHER_LATITUDE/LONGITUDE into
    // a human-readable place name (out.location), via BigDataCloud's free
    // no-API-key reverse-geocoding endpoint. The dashboard doesn't move, so
    // callers only need to call this once, ever, not on every wake. Same
    // success/failure contract as fetch().
    bool fetchLocationName(WeatherData &out);
};
