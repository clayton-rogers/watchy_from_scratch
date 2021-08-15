#include "clock.h"

#include <DS3232RTC.h> // RTC

// ******************************************************** //
// Globals
static DS3232RTC RTC(false);
static tmElements_t currentTime;
RTC_DATA_ATTR static bool alarm_enabled;


// ******************************************************** //
// Exported Functions
tmElements_t get_date_time() {
    return currentTime;
}

void set_date_time(const tmElements_t& new_time) {
    RTC.set(makeTime(new_time));
}

void first_time_rtc_config() {
    RTC.squareWave(SQWAVE_NONE); //disable square wave output
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
    RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    RTC.read(currentTime);
    alarm_enabled = true;
}

void clock_init() {
    RTC.alarm(ALARM_2); // reset alarm flag
    RTC.read(currentTime);
    if (!alarm_enabled) {
        RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
        alarm_enabled = true;
    }
}

void disable_minute_alarm() {
    RTC.alarmInterrupt(ALARM_2, false); //enable alarm interrupt
    alarm_enabled = false;
}
