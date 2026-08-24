#pragma once

#include "panel.h"
#include "../rtc/rtc_clock.h"

class ClockPanel : public Panel {
public:
    explicit ClockPanel(RtcClock &rtc) : rtc_(rtc) {}

    const char *title() const override { return "Clock"; }
    void drawContent(EpdDisplay &display, Rect_t area) override;
    void tick(EpdDisplay &display, Rect_t area) override;

private:
    struct Layout {
        int32_t timeY;
        int32_t dateY;
    };
    Layout computeLayout(Rect_t area) const;

    RtcClock &rtc_;
};
