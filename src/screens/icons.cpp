#include "icons.h"

#include <math.h>

namespace Icons {

void drawSun(EpdDisplay &display, int32_t cx, int32_t cy, int32_t r) {
    display.frameCircle(cx, cy, r, false);

    int32_t rayInner = r + 8;
    int32_t rayOuter = r + 22;
    for (int i = 0; i < 8; i++) {
        float angle = i * (3.14159265f / 4.0f); // 8 rays, 45 degrees apart
        int32_t x0 = cx + (int32_t)(rayInner * cosf(angle));
        int32_t y0 = cy + (int32_t)(rayInner * sinf(angle));
        int32_t x1 = cx + (int32_t)(rayOuter * cosf(angle));
        int32_t y1 = cy + (int32_t)(rayOuter * sinf(angle));
        display.frameLine(x0, y0, x1, y1);
    }
}

} // namespace Icons
