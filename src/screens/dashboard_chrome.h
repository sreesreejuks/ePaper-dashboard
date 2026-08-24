#pragma once

// Shared header/footer drawn around every screen's content, so individual
// Screen implementations never have to worry about branding/page-indicator
// layout -- only ScreenManager calls this.
#include "../display/epd_display.h"

namespace DashboardChrome {
void draw(EpdDisplay &display, const char *title, int pageIndex, int pageCount);
}
