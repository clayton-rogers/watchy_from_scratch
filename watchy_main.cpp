#include "watchy_main.h"

#include <Arduino.h>
#include <Wire.h>
#include <GxEPD2_BW.h> // screen
#include <DS3232RTC.h> // RTC
#include <bma.h>       // accelerometer
#include "battery.h"   // battery voltage monitor
#include "button.h"

// Fonts
#include <DSEG7_Classic_Bold_53.h> // Time
#include "Seven_Segment10pt7b.h"   // DoW
#include "DSEG7_Classic_Bold_25.h" // Date
#include "DSEG7_Classic_Regular_39.h" // Temp
#include <Fonts/FreeMonoBold9pt7b.h> // Menu
#include "icons.h"

//#include "DSEG7_Classic_Regular_15.h"

//#include "icons.h"


#include "pin_def.h"
#define YEAR_OFFSET 1970

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY));
static DS3232RTC RTC(false);
static tmElements_t currentTime;
static uint32_t step_count;
static int temperature_c;

RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR int steps_offset;


static void first_time_rtc_config() {
    RTC.squareWave(SQWAVE_NONE); //disable square wave output
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
    RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    RTC.read(currentTime);
}

uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)address, (uint8_t)len);
    uint8_t i = 0;
    while (Wire.available()) {
        data[i++] = Wire.read();
    }
    return 0;
}

uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data, len);
    return (0 !=  Wire.endTransmission());
}

static void first_time_bma_config() {
    if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
        //fail to init BMA
        return;
    }

    // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor.setAccelConfig(cfg);

    // Enable BMA423 accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    sensor.enableAccel();

    struct bma4_int_pin_config config ;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl = BMA4_ACTIVE_HIGH;
    config.od = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en = BMA4_INPUT_DISABLE;
    // The correct trigger interrupt needs to be configured as needed
    sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

    struct bma423_axes_remap remap_data;
    remap_data.x_axis = 1;
    remap_data.x_axis_sign = 0xFF;
    remap_data.y_axis = 0;
    remap_data.y_axis_sign = 0xFF;
    remap_data.z_axis = 2;
    remap_data.z_axis_sign = 0xFF;
    // Need to raise the wrist function, need to set the correct axis
    sensor.setRemapAxes(&remap_data);

    // Enable BMA423 isStepCounter feature
    sensor.enableFeature(BMA423_STEP_CNTR, true);
    // Enable BMA423 isTilt feature
    sensor.enableFeature(BMA423_TILT, true);
    // Enable BMA423 isDoubleClick feature
    sensor.enableFeature(BMA423_WAKEUP, true);

    // Reset steps
    sensor.resetStepCounter();

    // Turn on feature interrupt
    sensor.enableStepCountInterrupt();
    sensor.enableTiltInterrupt();
    // It corresponds to isDoubleClick interrupt
    sensor.enableWakeupInterrupt();
}

static void first_time_watch_init() {
    // nothing for now
    first_time_rtc_config();
    first_time_bma_config();
}

static void watch_init() {
    display.init(0, false);
    display.setFullWindow();
    RTC.alarm(ALARM_2); // reset alarm flag
    RTC.read(currentTime);
    step_count = sensor.getCounter() + steps_offset;
    temperature_c = RTC.temperature() / 4;
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
    display.drawBitmap(10, 165, steps, 19, 23, GxEPD_BLACK);
    display.setCursor(35, 190);
    display.println(step_count);

    // =================================
    // Draw Weather
    display.setFont(&DSEG7_Classic_Regular_39);
    display.getTextBounds(String(temperature_c), 100, 150, &x1, &y1, &w, &h);
    display.setCursor(155 - w, 150);
    display.println(temperature_c);
    display.drawBitmap(165, 110, celsius, 26, 20, GxEPD_BLACK);

    //int16_t weatherConditionCode = 800;
    const unsigned char* weatherIcon = sunny;
    ////https://openweathermap.org/weather-conditions
    //if(weatherConditionCode > 801){//Cloudy
    //weatherIcon = cloudy;
    //}else if(weatherConditionCode == 801){//Few Clouds
    //weatherIcon = cloudsun;
    //}else if(weatherConditionCode == 800){//Clear
    //weatherIcon = sunny;
    //}else if(weatherConditionCode >=700){//Atmosphere
    //weatherIcon = cloudy;
    //}else if(weatherConditionCode >=600){//Snow
    //weatherIcon = snow;
    //}else if(weatherConditionCode >=500){//Rain
    //weatherIcon = rain;
    //}else if(weatherConditionCode >=300){//Drizzle
    //weatherIcon = rain;
    //}else if(weatherConditionCode >=200){//Thunderstorm
    //weatherIcon = rain;
    //}else
    //return;

    const uint8_t WEATHER_ICON_WIDTH = 48;
    const uint8_t WEATHER_ICON_HEIGHT = 32;
    display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK);

    // =================================
    // Draw Battery
    display.setFont(&Seven_Segment10pt7b);
    display.setCursor(154, 93);
    display.printf("%d%%", (int)get_battery_percentage());

    // =================================
    // Flush to screen
    display.display(partial_refresh);
}

enum class Battery_Screen {
    NONE,
    READING,
    WAITING_FOR_CHARGE,
};
void draw_check_battery(Battery_Screen screen) {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    if (screen == Battery_Screen::READING) {
        display.setCursor(65, 30);
        display.println("Battery");
        display.setCursor(0, 80);
        display.print(" Voltage: ");
        display.print(get_battery_voltage());
        display.println(" V");
        display.print(" Offset:  ");
        display.print(get_adc_cal());
        display.println(" V");
        display.setCursor(20, 150);
        display.print("Press Menu to");
        display.setCursor(20, 170);
        display.print("calibrate");
    }
    if (screen == Battery_Screen::WAITING_FOR_CHARGE) {
        display.setCursor(0, 60);
        display.println("Wait for full");
        display.println("charge (red light");
        display.println("off) then unplug");
        display.println("then press Menu");
    }

    display.display(true);
}

void handle_check_battery() {

    Button b = Button::NONE;
    Battery_Screen screen = Battery_Screen::READING;
    Battery_Screen next_screen = Battery_Screen::NONE;
    while (1) {
        if (screen == Battery_Screen::READING) {
            if (b == Button::MENU) next_screen = Battery_Screen::WAITING_FOR_CHARGE;
            if (b == Button::BACK) break; // exit battery menu
            if (b == Button::UP)   set_adc_cal(get_adc_cal() + 0.01f);
            if (b == Button::DOWN) set_adc_cal(get_adc_cal() - 0.01f);
        }
        if (screen == Battery_Screen::WAITING_FOR_CHARGE) {
            if (b == Button::MENU) {
                // Do the auto calibration
                // We assume that the charging circuit accurately charges to 4.20 V
                set_adc_cal(0.0f); // clear any current calibration
                float voltage = get_battery_voltage();
                set_adc_cal(4.20f - voltage);
                next_screen = Battery_Screen::READING;
            }
            if (b == Button::BACK) next_screen = Battery_Screen::READING;
        }

        if (next_screen != Battery_Screen::NONE) {
            screen = next_screen;
            next_screen = Battery_Screen::NONE;
        }

        draw_check_battery(screen);
        b = get_next_button();
    }
}

static void handle_vibrate() {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(70, 80);
    display.println("Buzz!");
    display.display(true);

    pinMode(VIB_MOTOR_PIN, OUTPUT);
    bool motorOn = false;
    int length = 20;
    uint32_t interval_ms = 100;
    for(int i = 0; i < length; ++i){
        motorOn = !motorOn;
        digitalWrite(VIB_MOTOR_PIN, motorOn);
        delay(interval_ms);
    }
}

static void draw_set_time(tmElements_t time, int index) {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&DSEG7_Classic_Bold_53);

    display.setCursor(5, 80);
    if(time.Hour < 10){
        display.print("0");
    }
    display.print(time.Hour);

    display.print(":");

    display.setCursor(108, 80);
    if(time.Minute < 10){
        display.print("0");
    }
    display.print(time.Minute);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(45, 150);
    display.print(time.Year + YEAR_OFFSET);

    display.print("/");

    if(time.Month < 10){
        display.print("0");
    }
    display.print(time.Month);

    display.print("/");

    if(time.Day < 10){
        display.print("0");
    }
    display.print(time.Day);

    // Draw edit index
    {
        const int x_offset[] = {50, 150, 70, 110, 145};
        const int y_offset[] = {90, 90, 160, 160, 160};
        const int x = x_offset[index];
        const int y = y_offset[index];

        display.fillTriangle(x, y, x - 8, y + 8, x + 8, y + 8, GxEPD_WHITE);
    }

    display.display(true);
}

static void normalize_datetime(tmElements_t* tm) {
    if (tm->Month < 1)     tm->Month = 12;
    if (tm->Month > 12)    tm->Month = 1;
    if (tm->Day < 1)       tm->Day = 31;
    if (tm->Day > 31)      tm->Day = 1;
    if (tm->Hour == 255)   tm->Hour = 23;
    if (tm->Hour > 23)     tm->Hour = 0;
    if (tm->Minute == 255) tm->Minute = 59;
    if (tm->Minute > 59)   tm->Minute = 0;
    if (tm->Second == 255) tm->Second = 59;
    if (tm->Second > 59)   tm->Second = 0;
}

static void handle_set_time() {

    Button b = Button::NONE;
    tmElements_t new_time = currentTime;
    new_time.Second = 0;
    enum class Edit_Index {
        HOUR,
        MINUTE,
        YEAR,
        MONTH,
        DAY,
        MAX,
    };
    Edit_Index edit_index = Edit_Index::HOUR;
    while (1) {
        if (b == Button::MENU) {
            // dirty hack to increment enum class
            edit_index = (Edit_Index)((int)(edit_index) + 1);
            if (edit_index == Edit_Index::MAX) {
                RTC.set(makeTime(new_time));
                break;
            }
        }
        if (b == Button::BACK) {
            if (edit_index == Edit_Index::HOUR) {
                break; // exit without setting time
            }
            edit_index = (Edit_Index)((int)(edit_index) - 1);
        }
        if (b == Button::UP || b == Button::DOWN) {
            int inc = 0;
            if (b == Button::UP) {
                inc = 1;
            } else {
                inc = -1;
            }
            switch (edit_index) {
                case Edit_Index::HOUR:   new_time.Hour+=inc; break;
                case Edit_Index::MINUTE: new_time.Minute+=inc; break;
                case Edit_Index::YEAR:   new_time.Year+=inc; break;
                case Edit_Index::MONTH:  new_time.Month+=inc; break;
                case Edit_Index::DAY:    new_time.Day+=inc; break;
                case Edit_Index::MAX: break;
            }
            normalize_datetime(&new_time);
        }
        draw_set_time(new_time, (int)edit_index);

        b = get_next_button();
    }
}

static const int STEPS_WIDTH = 5;
static void draw_set_steps(const int steps, const int index) {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);

    // Print heading
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 50);
    display.print("Set Steps:");

    // Print the actual step count with leadin zeros
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(20,100);

    int temp_steps = steps / 10;
    for (int i = 0; i < STEPS_WIDTH-1; ++i) {
        if (temp_steps == 0) {
            display.print("0");
        } else {
            temp_steps /= 10;
        }
    }
    display.print(steps);

    // Print the index indicator
    const int x_offset[] = { 30, 50, 75, 95, 115 };
    const int y = 110;
    const int x = x_offset[index];

    display.fillTriangle(x, y, x - 8, y + 8, x + 8, y + 8, GxEPD_WHITE);

    display.display(true);
}

static void handle_set_steps() {
    Button b = Button::NONE;
    int index = 0;
    int local_steps = steps_offset;
    while (1) {
        if (b == Button::MENU) {
            index++;
            if (index == STEPS_WIDTH) {
                steps_offset = local_steps;
                break; // save and exit
            }
        }
        if (b == Button::BACK) {
            index--;
            if (index == -1) {
                break; // exit steps menu
            }
        }
        if (b == Button::UP || b == Button::DOWN) {
            int position = STEPS_WIDTH - index - 1;
            int add_val = 1;
            while (position-- > 0) {
                add_val *= 10;
            }
            if (b == Button::UP) {
                local_steps += add_val;
            } else {
                local_steps -= add_val;
                if (local_steps < 0) local_steps += add_val;
            }
        }

        draw_set_steps(local_steps, index);
        b = get_next_button();
    }

    step_count = sensor.getCounter() + steps_offset;
}

static void null_menu() {}

typedef void(*menu_ptr)();
static const char* menuItems[] =
    {"Check Battery", "Vibrate Motor", "Set Steps", "Set Time", "====", "===="};
menu_ptr menu_handlers[] =
    {handle_check_battery, handle_vibrate, handle_set_steps, handle_set_time, null_menu, null_menu };
#define MENU_HEIGHT 30
#define MENU_LENGTH 6
static void draw_menu(int menu_index, bool partial_refresh) {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t  x1, y1;
    uint16_t w, h;
    int16_t yPos;

    for (int i = 0; i < MENU_LENGTH; i++){
        yPos = 30+(MENU_HEIGHT*i);
        display.setCursor(0, yPos);
        if (i == menu_index) {
            display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
            display.fillRect(x1-1, y1-10, 200, h+15, GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            display.println(menuItems[i]);
        } else {
            display.setTextColor(GxEPD_WHITE);
            display.println(menuItems[i]);
        }
    }

    display.display(partial_refresh);
}

static void handle_menu() {

    int menu_index = 0;
    Button b = Button::NONE;
    while (1) {
        if (b == Button::MENU) menu_handlers[menu_index]();
        if (b == Button::BACK) break;
        if (b == Button::DOWN) {
            menu_index++;
            if (menu_index > MENU_LENGTH - 1) {
                menu_index = 0;
            }
        }
        if (b == Button::UP) {
            menu_index--;
            if (menu_index < 0) {
                menu_index = MENU_LENGTH - 1;
            }
        }
        draw_menu(menu_index, true);
        b = get_next_button();
    }
}

void run_watch() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason
    Wire.begin(SDA, SCL); //init i2c

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
            watch_init();
            display_watchface(true);
            break;
        case ESP_SLEEP_WAKEUP_EXT1: // Button press
        {
            watch_init();
            Button b = get_next_button();
            if (b == Button::MENU) {
                handle_menu();
                display_watchface(false);
            } else {
                display_watchface(true);
            }
            break;
        }
        default:
            first_time_watch_init();
            watch_init();
            display_watchface(false);
            break;
    }

    deep_sleep();
}
