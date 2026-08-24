#pragma once

#include "screen.h"
#include "../weather/weather_data.h"

class WeatherScreen : public Screen {
public:
    explicit WeatherScreen(const WeatherData &data) : data_(data) {}

    const char *name() const override { return "WEATHER"; }
    void drawContent(EpdDisplay &display) override;
    void tick(EpdDisplay &display) override {} // static for now; Phase 3 adds cache/staleness checks here

private:
    const WeatherData &data_;
};
