#pragma once

#include "panel.h"
#include "../weather/weather_data.h"

class WeatherPanel : public Panel {
public:
    explicit WeatherPanel(const WeatherData &data) : data_(data) {}

    const char *title() const override { return "Weather"; }
    void drawContent(EpdDisplay &display, Rect_t area) override;
    void tick(EpdDisplay &display, Rect_t area) override {} // static mock for now; Phase 3 adds real updates

private:
    const WeatherData &data_;
};
