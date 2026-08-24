#pragma once

// Plain data shape shared between whatever *produces* weather data (a
// hardcoded mock for now, a real WeatherService fetching from an API in
// Phase 3, a cached copy read back after a failed fetch) and WeatherScreen,
// which only ever reads from this struct and never knows where it came from.
struct WeatherData {
    float tempC;
    char condition[32];
    float highC;
    float lowC;
    int humidity;
    char location[40]; // reverse-geocoded from config.h's WEATHER_LATITUDE/LONGITUDE once at boot
    bool valid; // false = nothing to show (no mock/cache/fetch has succeeded yet)
};
