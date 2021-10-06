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
#include "datetime_utils.h"
#include "vibrate.h"
#include "settings.h"

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
static bool should_sleep = false;

#define INTERNET_UPDATE_INTERVAL 30 //minutes
RTC_DATA_ATTR int internet_updata_counter = INTERNET_UPDATE_INTERVAL;

static void update_from_internet_if_required(bool force = false) {

    if ((internet_updata_counter >= INTERNET_UPDATE_INTERVAL && get_settings().internet_update) || force) {
        internet_updata_counter = 0;

        if (connect_to_wifi()) {
            update_weather_data_from_internet();
            update_calendar_from_internet();
            disconnect_from_wifi();
        } else {
            Serial.println("Failed to connect to wifi");
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

    // Reset time, vibrate and reset step count
    const bool should_reset_steps = (currentTime.Hour == get_settings().reset_hour) && (currentTime.Minute == get_settings().reset_minute);
    if (should_reset_steps) {
        vibrate();
        // Wait for user to see screen
        delay(5000);
    }
    update_steps(should_reset_steps);
}

static void deep_sleep() {
    display.hibernate();
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
    esp_deep_sleep_start();
}

static void display_calendar() {
    display.setFont(&Seven_Segment10pt7b);

    const int base_cursor_x = 100;
    const int base_cursor_y = 135;
    const int newline = Seven_Segment10pt7b.yAdvance;
    const int MAX_MINUTES_BEFORE_EVENT_DISPLAY = 99;
    const int MINUTES_BEFORE_TO_VIBRATE = 10;

    calendar_event_t event = get_next_calendar_event();

    // Check that the event is in the next hour, otherwise don't care
    tmElements_t current_time = get_date_time();
    int mins_till_next_event = time_delta_minutes(current_time, event.start_time);
    if (mins_till_next_event > MAX_MINUTES_BEFORE_EVENT_DISPLAY || mins_till_next_event < -9999) {
        display.setCursor(base_cursor_x, base_cursor_y - newline);
        display.print("=========");
        display.setCursor(base_cursor_x, base_cursor_y);
        display.print("Free Time");
        return;
    }

    if (mins_till_next_event == MINUTES_BEFORE_TO_VIBRATE) {
        vibrate();
    }

    // Display the time and the countdown
    display.setCursor(base_cursor_x, base_cursor_y - newline);
    display.printf(" %02d:%02d", event.start_time.Hour, event.start_time.Minute);
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(base_cursor_x + 55, base_cursor_y - newline);
    display.print(mins_till_next_event);
    display.setFont(&Seven_Segment10pt7b);

    // Display the calendar event name
    bool done = false;
    int index = 0;
    for (int line = 0; line < 4 && !done; ++line) {
        display.setCursor(base_cursor_x, base_cursor_y + line*newline);
        for (int i = 0; i < 11 && !done; ++i) {
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
            }
        }
    }
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
    display.setCursor(90 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(90 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);

    // =================================
    // Draw Steps
    display.drawBitmap(0, 165, steps, 19, 23, GxEPD_BLACK);
    display.setCursor(25, 190);
    display.println(get_todays_steps());

    // =================================
    // Draw Weather
    WeatherData data = get_weather_data();
    //const unsigned char* weatherIcon = sunny;
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 150);
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

    //const uint8_t WEATHER_ICON_WIDTH = 48;
    //const uint8_t WEATHER_ICON_HEIGHT = 32;
    //display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK);

    // =================================
    // Draw Battery
    display.setFont(&Seven_Segment10pt7b);
    display.setCursor(140, 85);
    display.printf("%.1f%%", get_battery_percentage());

    // =================================
    // Draw Calendar
    display_calendar();

    // =================================
    // Flush to screen
    display.display(partial_refresh);
}

static void run_unit_tests() {
    Serial.begin(115200);

    Serial.println("Unit tests:");

    {
        tmElements_t first;
        first.Year = 51;
        first.Month = 8;
        first.Day = 31;
        first.Hour = 23;
        first.Minute = 59;
        first.Second = 0;
        tmElements_t second;
        second.Year = 51;
        second.Month = 9;
        second.Day = 1;
        second.Hour = 0;
        second.Minute = 1;
        second.Second = 0;

        int delta = time_delta_minutes(first, second);
        Serial.println(String("2 == ") + String(delta));
        delta = time_delta_minutes(second, first);
        Serial.println(String("-2 == ") + String(delta));


        first.Year = 51;
        first.Month = 8;
        first.Day = 31;
        first.Hour = 21;
        first.Minute = 23;
        first.Second = 0;

        second.Year = 51;
        second.Month = 8;
        second.Day = 31;
        second.Hour = 22;
        second.Minute = 23;
        second.Second = 0;

        delta = time_delta_minutes(first, second);
        Serial.println(String("60 == ") + String(delta));
    }

    Serial.println("End unit tests");
    Serial.flush();
}

static void display_sleep_face() {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, sleeeep, 200, 200, GxEPD_BLACK);
    display.display(false); // full refresh
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
            } else if (b == Button::UP) {
                // Force update from the internet
                update_from_internet_if_required(true);
                display_watchface(false);
            } else {
                display_watchface(true);
            }
            break;
        }
        default:
            run_unit_tests();
            first_time_watch_init();
            watch_init();
            update_from_internet_if_required();
            display_watchface(false);
            break;
    }

    // 01:00
    const int sleep_hour = 1;
    const int sleep_minute = 0;
    const tmElements_t current_time = get_date_time();
    if ( (current_time.Hour == sleep_hour &&
            current_time.Minute == sleep_minute) ||
            should_sleep) {
        display_sleep_face();
        disable_minute_alarm();
    }

    deep_sleep();
}

void set_should_sleep() {
    should_sleep = true;
}
