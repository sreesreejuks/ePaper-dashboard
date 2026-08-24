#include "clock_screen.h"
#include "../config/config.h"

#include "firasans.h"
#include "roboto32.h"

#include <cstring>

void ClockScreen::drawContent(EpdDisplay &display) {
    if (!rtc_.isOnline()) {
        display.frameText(&FiraSans, "RTC not found", TIME_CURSOR_X, DATE_CURSOR_Y);
        return;
    }

    display.frameText(&Roboto32, rtc_.timeString(), TIME_CURSOR_X, TIME_CURSOR_Y);

    const char *date = rtc_.dateString();
    strncpy(lastDate_, date, sizeof(lastDate_) - 1);
    lastDate_[sizeof(lastDate_) - 1] = '\0';
    display.frameText(&FiraSans, lastDate_, DATE_CURSOR_X, DATE_CURSOR_Y);
}

void ClockScreen::tick(EpdDisplay &display) {
    if (!rtc_.isOnline()) return;

    Rect_t timeArea = {TIME_AREA_X, TIME_AREA_Y, TIME_AREA_W, TIME_AREA_H};
    display.redrawTextInArea(timeArea, &Roboto32, rtc_.timeString(), TIME_CURSOR_X, TIME_CURSOR_Y);

    const char *date = rtc_.dateString();
    if (strcmp(date, lastDate_) != 0) {
        strncpy(lastDate_, date, sizeof(lastDate_) - 1);
        lastDate_[sizeof(lastDate_) - 1] = '\0';
        Rect_t dateArea = {DATE_AREA_X, DATE_AREA_Y, DATE_AREA_W, DATE_AREA_H};
        display.redrawTextInArea(dateArea, &FiraSans, lastDate_, DATE_CURSOR_X, DATE_CURSOR_Y);
    }
}
