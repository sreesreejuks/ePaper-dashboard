#pragma once

// Interface for one widget in the split-panel dashboard (TRMNL-style: several
// panels shown simultaneously side by side, not one full-screen page at a
// time). A panel only ever draws inside the Rect_t content area it's given
// by DashboardLayout -- it never touches chrome (border/footer) or knows
// where on the physical screen it actually sits. Adding a new plugin/content
// source is just a new Panel subclass, nothing else changes.
#include "../display/epd_display.h"

class Panel {
public:
    virtual ~Panel() {}

    // Footer label, e.g. "Clock", "Weather".
    virtual const char *title() const = 0;

    // Draw this panel's full content inside `area`. Called on boot and on
    // the periodic full-refresh ghost reset.
    virtual void drawContent(EpdDisplay &display, Rect_t area) = 0;

    // Called on every update while active; the panel decides whether/what
    // inside `area` actually needs a partial redraw.
    virtual void tick(EpdDisplay &display, Rect_t area) = 0;
};
