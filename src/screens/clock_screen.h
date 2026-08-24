#pragma once

#include "screen.h"
#include "../rtc/rtc_clock.h"

class ClockScreen : public Screen {
public:
    explicit ClockScreen(RtcClock &rtc) : rtc_(rtc) {}

    const char *name() const override { return "CLOCK"; }
    void drawContent(EpdDisplay &display) override;
    void tick(EpdDisplay &display) override;

private:
    RtcClock &rtc_;
    char lastDate_[16] = "";
};
