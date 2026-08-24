#pragma once

// Composes two Panels into the TRMNL-style split-screen layout: each panel
// gets a dashed-border frame and a gray footer strip with its title, and the
// panel only ever draws inside the content area this returns for it.
#include "panel.h"

namespace DashboardLayout {

Rect_t leftContentArea();
Rect_t rightContentArea();

void drawFull(EpdDisplay &display, Panel &left, Panel &right);
void tick(EpdDisplay &display, Panel &left, Panel &right);

} // namespace DashboardLayout
