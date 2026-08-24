#pragma once

// Small vector icons drawn with EpdDisplay's line/circle primitives -- no
// bitmap assets needed. Add more icons here as new panels need them.
#include "../display/epd_display.h"

namespace Icons {
void drawSun(EpdDisplay &display, int32_t cx, int32_t cy, int32_t r);
}
