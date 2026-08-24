#pragma once

// Thin wrapper around the existing, already-working LilyGo-EPD47 epd_driver
// API. This class does NOT replace or reimplement the display driver -- it
// only gives the rest of the app a small, testable surface to call instead
// of scattering epd_init()/epd_poweron()/writeln() calls across every screen.
#include "epd_driver.h"

class EpdDisplay {
public:
    // epd_init() + allocates the persistent PSRAM framebuffer used by the
    // frame* methods below (same ps_calloc pattern demo.ino already uses).
    // Returns false if the allocation failed.
    bool begin();
    void powerOn();
    void powerOff();
    void fullClear(); // epd_clear() -- aggressive flash-clear, resets ghosting

    // ---- Full-frame drawing ----
    // Chrome (header/footer/page dots) + a screen's content are composed
    // entirely in RAM, then pushed to the panel exactly once as a single
    // clean (non-flashing) pass that replaces every pixel -- correct
    // regardless of how different the previous screen's layout was. Used on
    // screen switches and after fullClear().
    void beginFrame();                                                          // clears the in-RAM framebuffer to white
    void frameText(const GFXfont *font, const char *text, int32_t x, int32_t y); // black text, no explicit background fill
    void frameTextCentered(const GFXfont *font, const char *text, int32_t y);    // x auto-computed to center on the panel
    void frameRect(int32_t x, int32_t y, int32_t w, int32_t h, bool filled);
    void frameFilledGray(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t gray); // gray: 0=black .. 255=white
    void frameHLine(int32_t x, int32_t y, int32_t length);
    void frameVLine(int32_t x, int32_t y, int32_t length);
    void frameLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    void frameCircle(int32_t x, int32_t y, int32_t r, bool filled);
    void frameDashedRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t dashLen, int32_t gapLen);
    void endFrame();                                                             // pushes the framebuffer to the panel

    // ---- Direct small-area drawing ----
    // Fast partial updates for ticking content (e.g. a clock's minute
    // field), unrelated to the full-frame framebuffer above.
    void drawText(const GFXfont *font, const char *text, int32_t x, int32_t y);
    void drawTextCentered(const GFXfont *font, const char *text, int32_t y); // x auto-computed to center on the panel

    // Whitewashes `area` with a single clean (non-flashing) push -- the
    // building block for erasing dynamic content before redrawing it.
    // `area.width` must be even (packed 2 pixels/byte, no padding nibble).
    void whitewashArea(Rect_t area);

    // Aggressive multi-cycle black/white flash-clear (epd_clear_area()),
    // scoped to `area` instead of the whole panel. Stronger reset than
    // whitewashArea() -- use it for a region updated often enough that a
    // single gentle push isn't fully resetting the e-paper's hysteresis
    // (e.g. a clock ticking every minute). Takes noticeably longer/visibly
    // flashes, so reserve it for regions updated at most every several
    // seconds, not every frame.
    void flashClearArea(Rect_t area);

    // Ghost-free partial update of a single line: whitewashes the whole
    // fixed `area` first, then draws -- old glyphs are fully erased even
    // when the new text is narrower than the old text. For multi-line
    // dynamic regions, call whitewashArea() once yourself, then drawText()
    // (or drawTextCentered()) for each line -- see drawClock() for an example.
    void redrawTextInArea(Rect_t area, const GFXfont *font, const char *text, int32_t x, int32_t y);

    // Pixel width `text` would render at in `font` -- used by the
    // *Centered() helpers, also useful for laying out other content.
    int32_t textWidth(const GFXfont *font, const char *text);

private:
    uint8_t *framebuffer_ = nullptr;
};
