#pragma once

#include "weather_data.h"

// Fetches live conditions from Open-Meteo (api.open-meteo.com) -- free, no
// API key required. Owns its own WiFi connect/disconnect cycle, the same
// pattern RtcClock::syncFromNtp() uses: connects only for the duration of
// the fetch, then disconnects, so it doesn't hold the radio on between
// updates.
class WeatherService {
public:
    // On success, fills `out` (setting out.valid = true) and returns true.
    // On any failure (WiFi, HTTP, parse), leaves `out` untouched and returns
    // false -- callers just keep showing the last-known-good data.
    bool fetch(WeatherData &out);

    // One-time reverse geocode of config.h's WEATHER_LATITUDE/LONGITUDE into
    // a human-readable place name (out.location), via BigDataCloud's free
    // no-API-key reverse-geocoding endpoint. The dashboard doesn't move, so
    // callers only need to call this once at boot, not on every fetch().
    // Same success/failure contract as fetch().
    bool fetchLocationName(WeatherData &out);
};
