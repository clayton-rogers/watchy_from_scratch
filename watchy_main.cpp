#include "watchy_main.h"

#include <Arduino.h>
#include <Wire.h>
#include <GxEPD2_BW.h> // screen
#include <DS3232RTC.h> // RTC
#include <bma.h>       // accelerometer

// Fonts
#include <DSEG7_Classic_Bold_53.h> // Time
#include "Seven_Segment10pt7b.h"   // DoW
#include "DSEG7_Classic_Bold_25.h" // Date

//#include "DSEG7_Classic_Regular_15.h"
//#include "DSEG7_Classic_Regular_39.h"
//#include "icons.h"


#include "pin_def.h"
#define YEAR_OFFSET 1970

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY));
static DS3232RTC RTC(false);
static tmElements_t currentTime;

static void first_time_rtc_config() {
    RTC.squareWave(SQWAVE_NONE); //disable square wave output
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
    RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    RTC.read(currentTime);
}

static void first_time_bma_config() {
    // TODO
}

static void first_time_watch_init() {
    // nothing for now
    first_time_rtc_config();
    first_time_bma_config();
}

static void watch_init() {
    display.init(0, false);
    display.setFullWindow();
    Wire.begin(SDA, SCL); // i2c for RTC and Accel
    RTC.alarm(ALARM_2); // reset alarm flag
    RTC.read(currentTime);
}

static void deep_sleep() {
    display.hibernate();
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
    esp_deep_sleep_start();
}

static void display_watchface(bool partial_refresh) {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    // =================================
    // Draw Time
    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(5, 53+5);
    if(currentTime.Hour < 10){
        display.print("0");
    }
    display.print(currentTime.Hour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }
    display.println(currentTime.Minute);

    // =================================
    // Draw Date
    display.setFont(&Seven_Segment10pt7b);

    int16_t  x1, y1;
    uint16_t w, h;

    String dayOfWeek = dayStr(currentTime.Wday);
    display.getTextBounds(dayOfWeek, 5, 85, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);
    display.setCursor(5, 150);
    display.println(currentTime.Year + YEAR_OFFSET);// offset from 1970, since year is stored in uint8_t

    // =================================
    // Draw Steps

    // =================================
    // Draw Weather

    // =================================
    // Draw Battery

    display.display(partial_refresh);
}

void run_watch() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
            watch_init();
            display_watchface(true);
            break;
        case ESP_SLEEP_WAKEUP_EXT1: // Button press
            // TODO
            watch_init();
            display_watchface(true);
            break;
        default:
            first_time_watch_init();
            display_watchface(false);
            break;
    }

    deep_sleep();
}
