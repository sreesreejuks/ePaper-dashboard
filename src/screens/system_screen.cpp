#include "system_screen.h"
#include "../config/config.h"

#include "firasans.h"

#include <cstdio>

namespace {
void formatUptime(char *buf, size_t len) {
    uint32_t s = millis() / 1000;
    uint32_t h = s / 3600;
    uint32_t m = (s % 3600) / 60;
    uint32_t sec = s % 60;
    snprintf(buf, len, "Uptime: %02u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)sec);
}
} // namespace

void SystemScreen::drawContent(EpdDisplay &display) {
    char buf[64];

    snprintf(buf, sizeof(buf), "Chip: %s rev%d", ESP.getChipModel(), ESP.getChipRevision());
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE1_Y);

    snprintf(buf, sizeof(buf), "CPU: %u MHz   Flash: %u MB",
             (unsigned)ESP.getCpuFreqMHz(), (unsigned)(ESP.getFlashChipSize() / (1024 * 1024)));
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE2_Y);

    snprintf(buf, sizeof(buf), "Free heap: %u KB", (unsigned)(ESP.getFreeHeap() / 1024));
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE3_Y);

    snprintf(buf, sizeof(buf), "Free PSRAM: %u KB", (unsigned)(ESP.getFreePsram() / 1024));
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE4_Y);

    snprintf(buf, sizeof(buf), "RTC: %s", rtc_.isOnline() ? "Online" : "Offline");
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE5_Y);

    formatUptime(buf, sizeof(buf));
    display.frameText(&FiraSans, buf, SYSTEM_LINE_X, SYS_LINE6_Y);
}

void SystemScreen::tick(EpdDisplay &display) {
    char buf[64];
    formatUptime(buf, sizeof(buf));
    Rect_t area = {SYS_UPTIME_AREA_X, SYS_UPTIME_AREA_Y, SYS_UPTIME_AREA_W, SYS_UPTIME_AREA_H};
    display.redrawTextInArea(area, &FiraSans, buf, SYSTEM_LINE_X, SYS_LINE6_Y);
}
