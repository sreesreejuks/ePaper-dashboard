#include "weather_screen.h"
#include "../config/config.h"

#include "firasans.h"
#include "roboto18.h"
#include "roboto32.h"

#include <cstdio>

void WeatherScreen::drawContent(EpdDisplay &display) {
    if (!data_.valid) {
        display.frameText(&FiraSans, "Weather unavailable", WEATHER_TEMP_X, WEATHER_COND_Y);
        return;
    }

    // Degree symbol deliberately avoided -- not yet verified present in
    // every font's glyph table on real hardware, plain "C" is guaranteed ASCII.
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f C", data_.tempC);
    display.frameText(&Roboto32, buf, WEATHER_TEMP_X, WEATHER_TEMP_Y);

    display.frameText(&Roboto18, data_.condition, WEATHER_COND_X, WEATHER_COND_Y);

    snprintf(buf, sizeof(buf), "High %.0f  Low %.0f", data_.highC, data_.lowC);
    display.frameText(&FiraSans, buf, WEATHER_RANGE_X, WEATHER_RANGE_Y);

    snprintf(buf, sizeof(buf), "Humidity %d%%", data_.humidity);
    display.frameText(&FiraSans, buf, WEATHER_HUMIDITY_X, WEATHER_HUMIDITY_Y);
}
