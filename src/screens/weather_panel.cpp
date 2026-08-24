#include "weather_panel.h"
#include "icons.h"

#include "firasans.h"
#include "roboto12.h"
#include "roboto18.h"
#include "roboto32.h"

#include <cstdio>

void WeatherPanel::drawContent(EpdDisplay &display, Rect_t area) {
    display.frameText(&Roboto12, data_.location, area.x, area.y + 14);

    int32_t centerX = area.x + area.width / 2;

    if (!data_.valid) {
        const char *msg = "Weather unavailable";
        int32_t mw = display.textWidth(&FiraSans, msg);
        display.frameText(&FiraSans, msg, centerX - mw / 2, area.y + area.height / 2);
        return;
    }

    // Single centered column below the location label: icon, then the
    // temperature (kept the most prominent element), then the condition
    // text last so it has the full panel width to fit in -- the previous
    // side-by-side layout only gave it half the width, which is what let
    // long strings like "Thunderstorm w/ Hail" run past the panel edge.
    int32_t iconCy = area.y + area.height * 2 / 5;
    Icons::drawSun(display, centerX, iconCy, 40);

    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.0f C", data_.tempC);
    int32_t tw = display.textWidth(&Roboto32, tempBuf);
    int32_t tempY = iconCy + 68;
    display.frameText(&Roboto32, tempBuf, centerX - tw / 2, tempY);

    // Condition text: try Roboto18, but drop to Roboto12 if it would run
    // past the available width -- guarantees it never touches the panel's
    // dashed border regardless of how long the condition string is.
    int32_t maxCondWidth = area.width - 16; // small side margin, both edges
    const GFXfont *condFont = &Roboto18;
    int32_t cw = display.textWidth(condFont, data_.condition);
    if (cw > maxCondWidth) {
        condFont = &Roboto12;
        cw = display.textWidth(condFont, data_.condition);
    }
    int32_t condY = tempY + 38;
    display.frameText(condFont, data_.condition, centerX - cw / 2, condY);
}
