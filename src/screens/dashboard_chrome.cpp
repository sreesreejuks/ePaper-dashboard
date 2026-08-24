#include "dashboard_chrome.h"
#include "../config/config.h"

#include "roboto18.h"
#include "roboto12.h"

namespace DashboardChrome {

void draw(EpdDisplay &display, const char *title, int pageIndex, int pageCount) {
    display.frameText(&Roboto18, title, HEADER_TITLE_X, HEADER_TITLE_Y);
    display.frameHLine(CONTENT_MARGIN_X, HEADER_RULE_Y, EPD_WIDTH - 2 * CONTENT_MARGIN_X);

    // Page indicator dots, right-aligned in the header; the active page is filled.
    int32_t rightEdge = EPD_WIDTH - CONTENT_MARGIN_X - PAGE_DOT_RADIUS;
    int32_t firstCx = rightEdge - (pageCount - 1) * PAGE_DOT_SPACING;
    for (int i = 0; i < pageCount; i++) {
        int32_t cx = firstCx + i * PAGE_DOT_SPACING;
        display.frameCircle(cx, PAGE_DOT_Y, PAGE_DOT_RADIUS, i == pageIndex);
    }

    display.frameHLine(CONTENT_MARGIN_X, FOOTER_RULE_Y, EPD_WIDTH - 2 * CONTENT_MARGIN_X);
    display.frameText(&Roboto12, "LILYGO T5 Dashboard", CONTENT_MARGIN_X, FOOTER_TEXT_Y);
}

} // namespace DashboardChrome
