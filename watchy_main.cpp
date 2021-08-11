#include "watchy_main.h"

#include <Arduino.h>
#include <Wire.h>
#include <GxEPD2_BW.h> // screen


#include "battery.h"   // battery voltage monitor
#include "button.h"
#include "weather.h"
#include "wifi_wrapper.h"
#include "main_menu.h"
#include "steps.h"
#include "clock.h"
#include "calendar.h"

// Fonts
#include <DSEG7_Classic_Bold_53.h> // Time
#include "Seven_Segment10pt7b.h"   // DoW
#include "DSEG7_Classic_Bold_25.h" // Date
#include "DSEG7_Classic_Regular_39.h" // Temp
#include "icons.h"

//#include "DSEG7_Classic_Regular_15.h"

#include "pin_def.h"

#define YEAR_OFFSET 1970

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY));

#define INTERNET_UPDATE_INTERVAL 30 //minutes
RTC_DATA_ATTR int internet_updata_counter = INTERNET_UPDATE_INTERVAL;

static void update_from_internet_if_required() {
    if (internet_updata_counter >= INTERNET_UPDATE_INTERVAL) {
        internet_updata_counter = 0;

        if (connect_to_wifi()) {
            update_weather_data_from_internet();
            update_calendar_from_internet();
            disconnect_from_wifi();
        }

    } else {
        internet_updata_counter++;
    }
}

static void first_time_watch_init() {
    first_time_rtc_config();
    first_time_bma_config();
}

static void watch_init() {
    display.init(0, false);
    display.setFullWindow();
    clock_init();
    tmElements_t currentTime = get_date_time();
    const bool is_midnight = (currentTime.Hour == 0) && (currentTime.Minute == 0);
    update_steps(is_midnight);
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

    tmElements_t currentTime = get_date_time();

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
    //display.setCursor(5, 150);
    //display.println(currentTime.Year + YEAR_OFFSET);// offset from 1970, since year is stored in uint8_t

    // =================================
    // Draw Steps
    display.drawBitmap(10, 165, steps, 19, 23, GxEPD_BLACK);
    display.setCursor(35, 190);
    display.println(get_todays_steps());

    // =================================
    // Draw Weather
    WeatherData data = get_weather_data();
    //const unsigned char* weatherIcon = sunny;

    //display.setFont(&DSEG7_Classic_Regular_39);
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 150);
    //display.getTextBounds(String(data.temperature), 100, 150, &x1, &y1, &w, &h);
    //display.setCursor(155 - w, 150);
    display.print(data.temperature + String(" C"));
    //display.drawBitmap(165, 110, celsius, 26, 20, GxEPD_BLACK);


    ////https://openweathermap.org/weather-conditions
    //if (data.weather_condition_code > 801) {//Cloudy
    //    weatherIcon = cloudy;
    //} else if (data.weather_condition_code == 802) {//Few Clouds
    //    weatherIcon = cloudsun;
    //} else if (data.weather_condition_code == 800) {//Clear
    //    weatherIcon = sunny;
    //} else if (data.weather_condition_code >=700) {//Atmosphere
    //    weatherIcon = cloudy;
    //} else if (data.weather_condition_code >=600) {//Snow
    //    weatherIcon = snow;
    //} else if (data.weather_condition_code >=500) {//Rain
    //    weatherIcon = rain;
    //} else if (data.weather_condition_code >=300) {//Drizzle
    //    weatherIcon = rain;
    //} else if (data.weather_condition_code >=200) {//Thunderstorm
    //    weatherIcon = rain;
    //}
//
    //const uint8_t WEATHER_ICON_WIDTH = 48;
    //const uint8_t WEATHER_ICON_HEIGHT = 32;
    //display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK);

    // =================================
    // Draw Battery
    display.setFont(&Seven_Segment10pt7b);
    display.setCursor(140, 93);
    display.printf("%.1f%%", get_battery_percentage());

    // =================================
    // Draw Calendar
    calendar_event_t event = get_next_calendar_event();
    display.setFont(&Seven_Segment10pt7b);
    int index = 0;
    bool done = false;
    display.setCursor(100, 113);
    if (!done) {
        for (int i = 0; i < 11; ++i) {
            char c = event.name[index];
            if (c == 0) {
                done = true;
                break;
            }
            // if we find a space, put the next word
            // on the next line
            if (c == ' ') {
                ++index;
                break;
            }
            display.write(c);
            ++index;
            if (index == CALENDAR_EVENT_NAME_LENGTH) {
                done = true;
                break;
            }
        }
    }
    display.setCursor(100, 133);
    if (!done) {
        for (int i = 0; i < 11; ++i) {
            char c = event.name[index];
            if (c == 0) {
                done = true;
                break;
            }
            // if we find a space, put the next word
            // on the next line
            if (c == ' ') {
                ++index;
                break;
            }
            display.write(c);
            ++index;
            if (index == CALENDAR_EVENT_NAME_LENGTH) {
                done = true;
                break;
            }
        }
    }
    display.setCursor(100, 153);
    if (!done) {
        for (int i = 0; i < 11; ++i) {
            char c = event.name[index];
            if (c == 0) {
                done = true;
                break;
            }
            // if we find a space, put the next word
            // on the next line
            if (c == ' ') {
                ++index;
                break;
            }
            display.write(c);
            ++index;
            if (index == CALENDAR_EVENT_NAME_LENGTH) {
                done = true;
                break;
            }
        }
    }

    // =================================
    // Flush to screen
    display.display(partial_refresh);
}

void run_watch() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason
    Wire.begin(SDA, SCL); //init i2c

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
            watch_init();
            update_from_internet_if_required();
            display_watchface(true);
            break;
        case ESP_SLEEP_WAKEUP_EXT1: // Button press
        {
            watch_init();
            Button b = get_next_button();
            if (b == Button::MENU) {
                handle_main_menu();
                display_watchface(false);
            } else {
                update_from_internet_if_required();
                display_watchface(true);
            }
            break;
        }
        default:
            first_time_watch_init();
            watch_init();
            update_from_internet_if_required();
            display_watchface(false);
            break;
    }

    deep_sleep();
}
