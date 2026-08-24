#pragma once

#include "screen.h"

// Decides which Screen is currently shown, rotates between them on a timer,
// and owns the full-frame draw lifecycle (chrome + content) so individual
// screens stay simple.
class ScreenManager {
public:
    ScreenManager(Screen **screens, int count, uint32_t rotationIntervalMs);

    void begin(EpdDisplay &display);            // draws the first screen
    void update(EpdDisplay &display);           // call every TICK_INTERVAL_MS
    void forceFullRefresh(EpdDisplay &display); // periodic ghost-reset: flash-clear + redraw current screen

private:
    void drawCurrentFull(EpdDisplay &display);

    Screen **screens_;
    int count_;
    int current_ = 0;
    uint32_t rotationIntervalMs_;
    uint32_t nextRotation_ = 0;
};
