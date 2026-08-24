#include "dashboard_layout.h"
#include "../config/config.h"

#include "roboto12.h"

namespace DashboardLayout {

namespace {

void drawPanelChrome(EpdDisplay &display, int32_t panelX, const char *label) {
    display.frameDashedRect(panelX, PANEL_Y, PANEL_W, PANEL_H, DASH_LEN, GAP_LEN);

    int32_t footerY = PANEL_Y + PANEL_H - FOOTER_H;
    display.frameFilledGray(panelX, footerY, PANEL_W, FOOTER_H, FOOTER_GRAY_LEVEL);
    display.frameText(&Roboto12, label, panelX + PANEL_PADDING, footerY + FOOTER_H / 2 + 6);
}

} // namespace

Rect_t leftContentArea() {
    return {PANEL_LEFT_X + PANEL_PADDING, PANEL_Y + PANEL_PADDING,
            PANEL_W - 2 * PANEL_PADDING, PANEL_H - FOOTER_H - PANEL_PADDING};
}

Rect_t rightContentArea() {
    return {PANEL_RIGHT_X + PANEL_PADDING, PANEL_Y + PANEL_PADDING,
            PANEL_W - 2 * PANEL_PADDING, PANEL_H - FOOTER_H - PANEL_PADDING};
}

void drawFull(EpdDisplay &display, Panel &left, Panel &right) {
    display.beginFrame();

    drawPanelChrome(display, PANEL_LEFT_X, left.title());
    drawPanelChrome(display, PANEL_RIGHT_X, right.title());

    left.drawContent(display, leftContentArea());
    right.drawContent(display, rightContentArea());

    display.endFrame();
}

void tick(EpdDisplay &display, Panel &left, Panel &right) {
    left.tick(display, leftContentArea());
    right.tick(display, rightContentArea());
}

} // namespace DashboardLayout
