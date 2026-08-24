#pragma once

#include "screen.h"
#include "../rtc/rtc_clock.h"

class SystemScreen : public Screen {
public:
    explicit SystemScreen(RtcClock &rtc) : rtc_(rtc) {}

    const char *name() const override { return "SYSTEM"; }
    void drawContent(EpdDisplay &display) override;
    void tick(EpdDisplay &display) override;

private:
    RtcClock &rtc_;
};
