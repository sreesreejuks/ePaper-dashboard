#include "clock_panel.h"

#include "firasans.h"
#include "roboto32.h"

ClockPanel::Layout ClockPanel::computeLayout(Rect_t area) const {
    // timeY/dateY are tuned by eye (no font-metrics API exposed by
    // EpdDisplay beyond textWidth()) so the time+date pair reads as
    // vertically centered as a block within `area`, not just individually.
    int32_t timeY = area.y + area.height / 2 - 25;
    return {timeY, timeY + 55};
}

void ClockPanel::drawContent(EpdDisplay &display, Rect_t area) {
    if (!rtc_.isOnline()) {
        display.frameText(&FiraSans, "RTC not found", area.x, area.y + area.height / 2);
        return;
    }

    Layout layout = computeLayout(area);

    const char *time = rtc_.shortTimeString();
    int32_t tw = display.textWidth(&Roboto32, time);
    int32_t tx = area.x + (area.width - tw) / 2;
    // Faux-bold: Roboto32 is already the largest bundled bitmap font, so a
    // 1px-offset double-draw is used to give the time more visual weight
    // instead of a true size increase (which would need a new font asset).
    display.frameText(&Roboto32, time, tx, layout.timeY);
    display.frameText(&Roboto32, time, tx + 1, layout.timeY);

    const char *date = rtc_.longDateString();
    int32_t dw = display.textWidth(&FiraSans, date);
    display.frameText(&FiraSans, date, area.x + (area.width - dw) / 2, layout.dateY);
}

void ClockPanel::tick(EpdDisplay &display, Rect_t area) {
    if (!rtc_.isOnline()) return;

    Layout layout = computeLayout(area);

    // Flash-clear the whole time+date region as one unit before redrawing.
    // This panel updates only once a minute, so the stronger multi-cycle
    // clear (vs. whitewashArea's single gentle push) is cheap here and
    // reliably prevents leftover/overlapping digits.
    Rect_t dynamicArea = {area.x, layout.timeY - 90, area.width, 190};
    if (dynamicArea.width % 2 != 0) dynamicArea.width -= 1; // must be even (packed 2px/byte)
    display.flashClearArea(dynamicArea);

    const char *time = rtc_.shortTimeString();
    int32_t tw = display.textWidth(&Roboto32, time);
    int32_t tx = area.x + (area.width - tw) / 2;
    display.drawText(&Roboto32, time, tx, layout.timeY);
    display.drawText(&Roboto32, time, tx + 1, layout.timeY); // faux-bold, see drawContent()

    const char *date = rtc_.longDateString();
    int32_t dw = display.textWidth(&FiraSans, date);
    display.drawText(&FiraSans, date, area.x + (area.width - dw) / 2, layout.dateY);
}
