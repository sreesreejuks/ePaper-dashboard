#include "rtc_clock.h"

#include "utilities.h" // BOARD_SDA, BOARD_SCL
#include "../config/config.h" // TIMEZONE_OFFSET_MINUTES, WIFI_SSID, ...
#include <cstdio>
#include <WiFi.h>

namespace {
const char *kMonthNames[] = {"January", "February", "March", "April", "May", "June",
                              "July", "August", "September", "October", "November", "December"};

struct LocalTime {
    int year, month, day, hour, minute, second;
};

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// SensorRTC::adjustDate() exists in the library but is private, so this is
// our own small leap-year-aware day increment/decrement instead.
void addDays(int &year, int &month, int &day, int delta) {
    while (delta > 0) {
        day++;
        if (day > daysInMonth(year, month)) {
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
        delta--;
    }
    while (delta < 0) {
        day--;
        if (day < 1) {
            month--;
            if (month < 1) {
                month = 12;
                year--;
            }
            day = daysInMonth(year, month);
        }
        delta++;
    }
}

// Applies TIMEZONE_OFFSET_MINUTES to a raw RTC reading for display purposes
// only -- the RTC chip itself is left untouched, it just keeps ticking
// whatever wall-clock time it was last set to.
LocalTime applyOffset(RTC_DateTime dt, int offsetMinutes) {
    int year = dt.getYear();
    int month = dt.getMonth();
    int day = dt.getDay();
    int totalMinutes = dt.getHour() * 60 + dt.getMinute() + offsetMinutes;

    int dayCarry = 0;
    while (totalMinutes < 0) {
        totalMinutes += 24 * 60;
        dayCarry--;
    }
    while (totalMinutes >= 24 * 60) {
        totalMinutes -= 24 * 60;
        dayCarry++;
    }
    if (dayCarry != 0) {
        addDays(year, month, day, dayCarry);
    }

    return {year, month, day, totalMinutes / 60, totalMinutes % 60, dt.getSecond()};
}
} // namespace

bool RtcClock::begin() {
    Wire.begin(BOARD_SDA, BOARD_SCL);
    online_ = rtc_.begin(Wire, BOARD_SDA, BOARD_SCL);

    if (online_ && rtc_.getDateTime().getYear() < 2024) {
        // First boot / dead backup battery: seed from the sketch's compile time.
        // Phase 2 will replace this with periodic NTP sync over Wi-Fi. This
        // check is on the RAW (un-offset) value -- it's just asking "has this
        // chip ever been set at all", not "is it showing correct local time".
        //
        // __DATE__/__TIME__ reflect the *local* clock of the machine that
        // compiled this sketch (IST), but every read-side method below adds
        // TIMEZONE_OFFSET_MINUTES back on top of the raw RTC value. Storing
        // the local compile time as-is would double-count the offset and
        // make the display run 5:30 fast, so subtract it here to keep the
        // RTC's raw value in UTC.
        LocalTime utc = applyOffset(RTC_DateTime(__DATE__, __TIME__), -TIMEZONE_OFFSET_MINUTES);
        rtc_.setDateTime(RTC_DateTime(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second));
    }
    return online_;
}

bool RtcClock::syncFromNtp() {
    if (!online_) return false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < NTP_SYNC_TIMEOUT_MS) {
        delay(250);
    }

    bool synced = false;
    if (WiFi.status() == WL_CONNECTED) {
        // gmtOffset=0, daylightOffset=0: we want raw UTC out of NTP, since
        // the RTC chip always stores UTC -- TIMEZONE_OFFSET_MINUTES is
        // applied separately at display time (see applyOffset() above).
        configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);

        struct tm timeinfo;
        uint32_t remaining = NTP_SYNC_TIMEOUT_MS - (millis() - start);
        if (getLocalTime(&timeinfo, remaining)) {
            rtc_.setDateTime(RTC_DateTime(timeinfo));
            synced = true;
        }
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return synced;
}

// NOTE: deliberately NOT using SensorPCF8563::strftime()/formatDateTime().
// That library function always calls snprintf(fmt, year, month, day, hour,
// minute, second, weekday) in that fixed order regardless of which `fmt`
// you pass it -- so DT_FMT_HM ("%02d:%02d") actually prints year:month, and
// DT_FMT_HMS prints year:month:day, not the time. DT_FMT_YMD happens to
// consume args in matching order so it looks correct, but only by
// coincidence. Building each string from the individual getters below
// sidesteps that bug entirely and is correct for any format.

const char *RtcClock::timeString() {
    LocalTime lt = applyOffset(rtc_.getDateTime(), TIMEZONE_OFFSET_MINUTES);
    static char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", lt.hour, lt.minute, lt.second);
    return buf;
}

const char *RtcClock::dateString() {
    LocalTime lt = applyOffset(rtc_.getDateTime(), TIMEZONE_OFFSET_MINUTES);
    static char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", lt.year, lt.month, lt.day);
    return buf;
}

const char *RtcClock::shortTimeString() {
    LocalTime lt = applyOffset(rtc_.getDateTime(), TIMEZONE_OFFSET_MINUTES);
    static char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", lt.hour, lt.minute);
    return buf;
}

const char *RtcClock::longDateString() {
    LocalTime lt = applyOffset(rtc_.getDateTime(), TIMEZONE_OFFSET_MINUTES);
    const char *monthName = (lt.month >= 1 && lt.month <= 12) ? kMonthNames[lt.month - 1] : "?";
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d %s %d", lt.day, monthName, lt.year);
    return buf;
}
