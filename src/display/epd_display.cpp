#include "epd_display.h"

#include <Arduino.h> // ps_calloc()
#include <cstring>
#include <cstdlib>

bool EpdDisplay::begin() {
    epd_init();
    framebuffer_ = (uint8_t *)ps_calloc(1, (size_t)EPD_WIDTH * EPD_HEIGHT / 2);
    return framebuffer_ != nullptr;
}

void EpdDisplay::powerOn() { epd_poweron(); }
void EpdDisplay::powerOff() { epd_poweroff(); }
void EpdDisplay::fullClear() { epd_clear(); }

void EpdDisplay::beginFrame() {
    memset(framebuffer_, 0xFF, (size_t)EPD_WIDTH * EPD_HEIGHT / 2);
}

void EpdDisplay::frameText(const GFXfont *font, const char *text, int32_t x, int32_t y) {
    int32_t cursor_x = x;
    int32_t cursor_y = y;
    writeln(font, text, &cursor_x, &cursor_y, framebuffer_);
}

void EpdDisplay::frameTextCentered(const GFXfont *font, const char *text, int32_t y) {
    int32_t x = (EPD_WIDTH - textWidth(font, text)) / 2;
    frameText(font, text, x, y);
}

void EpdDisplay::frameRect(int32_t x, int32_t y, int32_t w, int32_t h, bool filled) {
    if (filled) {
        epd_fill_rect(x, y, w, h, 0, framebuffer_);
    } else {
        epd_draw_rect(x, y, w, h, 0, framebuffer_);
    }
}

void EpdDisplay::frameFilledGray(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t gray) {
    epd_fill_rect(x, y, w, h, gray, framebuffer_);
}

void EpdDisplay::frameHLine(int32_t x, int32_t y, int32_t length) {
    epd_draw_hline(x, y, length, 0, framebuffer_);
}

void EpdDisplay::frameVLine(int32_t x, int32_t y, int32_t length) {
    epd_draw_vline(x, y, length, 0, framebuffer_);
}

void EpdDisplay::frameLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    epd_draw_line(x0, y0, x1, y1, 0, framebuffer_);
}

void EpdDisplay::frameCircle(int32_t x, int32_t y, int32_t r, bool filled) {
    if (filled) {
        epd_fill_circle(x, y, r, 0, framebuffer_);
    } else {
        epd_draw_circle(x, y, r, 0, framebuffer_);
    }
}

void EpdDisplay::frameDashedRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t dashLen, int32_t gapLen) {
    int32_t step = dashLen + gapLen;
    for (int32_t px = 0; px < w; px += step) {
        int32_t seg = min(dashLen, w - px);
        frameHLine(x + px, y, seg);
        frameHLine(x + px, y + h, seg);
    }
    for (int32_t py = 0; py < h; py += step) {
        int32_t seg = min(dashLen, h - py);
        frameVLine(x, y + py, seg);
        frameVLine(x + w, y + py, seg);
    }
}

void EpdDisplay::endFrame() {
    epd_draw_grayscale_image(epd_full_screen(), framebuffer_);
}

void EpdDisplay::drawText(const GFXfont *font, const char *text, int32_t x, int32_t y) {
    int32_t cursor_x = x;
    int32_t cursor_y = y;
    writeln(font, text, &cursor_x, &cursor_y, NULL);
}

void EpdDisplay::drawTextCentered(const GFXfont *font, const char *text, int32_t y) {
    int32_t x = (EPD_WIDTH - textWidth(font, text)) / 2;
    drawText(font, text, x, y);
}

void EpdDisplay::whitewashArea(Rect_t area) {
    size_t bufSize = (size_t)(area.width / 2) * area.height;
    uint8_t *whiteBuf = (uint8_t *)malloc(bufSize);
    if (whiteBuf) {
        memset(whiteBuf, 0xFF, bufSize); // 0xFF = white for both packed pixels/byte
        epd_draw_image(area, whiteBuf, BLACK_ON_WHITE);
        free(whiteBuf);
    }
}

void EpdDisplay::flashClearArea(Rect_t area) {
    epd_clear_area(area);
}

void EpdDisplay::redrawTextInArea(Rect_t area, const GFXfont *font, const char *text, int32_t x, int32_t y) {
    whitewashArea(area);
    drawText(font, text, x, y);
}

int32_t EpdDisplay::textWidth(const GFXfont *font, const char *text) {
    int32_t x = 0, y = 0, x1 = 0, y1 = 0, w = 0, h = 0;
    get_text_bounds(font, text, &x, &y, &x1, &y1, &w, &h, NULL);
    return w;
}
