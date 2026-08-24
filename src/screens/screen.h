#pragma once

// Common interface every dashboard page implements. A screen never touches
// framebuffer lifecycle (beginFrame/endFrame) or draws chrome itself -- that
// is ScreenManager's job -- so adding a new plugin/content source only ever
// means writing a new Screen subclass, nothing else changes.
#include "../display/epd_display.h"

class Screen {
public:
    virtual ~Screen() {}

    // Short page name shown in the header (e.g. "CLOCK", "WEATHER").
    virtual const char *name() const = 0;

    // Draw only this screen's content into the already-open frame (chrome is
    // drawn separately by ScreenManager). Called on screen entry and on the
    // periodic full-refresh ghost reset.
    virtual void drawContent(EpdDisplay &display) = 0;

    // Called on every tick while this screen is active; the screen decides
    // whether/what actually needs a partial redraw (e.g. a clock's seconds).
    virtual void tick(EpdDisplay &display) = 0;
};
