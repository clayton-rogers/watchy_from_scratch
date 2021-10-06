#include "main_menu.h"

#include <Arduino.h>

#include "generic_menu.h"
#include "button.h"
#include "steps.h"
#include "watchy_display.h"
#include "datetime_utils.h"
#include "clock.h"
#include "pin_def.h"
#include "battery.h"
#include "vibrate.h"
#include "settings.h"
#include "watchy_main.h"

#include <Fonts/FreeMonoBold9pt7b.h> // Menu
#include <DSEG7_Classic_Bold_53.h> // Time
#include "DSEG7_Classic_Bold_25.h" // Date


// ******************************************************** //
// GLOBALS
#define YEAR_OFFSET 1970

// ******************************************************** //
// Battery screen
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

// ******************************************************** //
// Vibrate
static void handle_vibrate() {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(70, 80);
    display.println("Buzz!");
    display.display(true);

    vibrate();
}

// ******************************************************** //
// Set Time
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

static void handle_set_time() {

    Button b = Button::NONE;
    tmElements_t new_time = get_date_time();
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
                set_date_time(new_time);
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

// ******************************************************** //
// Steps
static const int STEPS_WIDTH = 5;
static void draw_set_steps(const int steps, const int index) {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);

    // Print heading
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 30);
    display.print("Set Steps:");

    // Print the actual step count with leadin zeros
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(20,80);

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
    const int y = 90;
    const int x = x_offset[index];

    display.fillTriangle(x, y, x - 8, y + 8, x + 8, y + 8, GxEPD_WHITE);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 130);
    display.print("Total steps:");
    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(20, 180);
    display.print(get_total_steps());

    display.display(true);
}

static void handle_set_steps() {
    Button b = Button::NONE;
    int index = 0;
    int local_steps = get_step_offset();
    while (1) {
        if (b == Button::MENU) {
            index++;
            if (index == STEPS_WIDTH) {
                set_step_offset(local_steps);
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
}

static void handle_sleep() {
    set_should_sleep();

    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0,30);
    display.print("Will Sleep!");
    display.display(true);

    delay(1000);
}

// ******************************************************** //
// Menu
static const char* menu_labels[] =
    {"Check Battery", "Settings", "Set Time", "Set Steps", "Vibrate Motor", "Sleep",};
static menu_handler_ptr menu_callbacks[] =
    {handle_check_battery, handle_settings_menu, handle_set_time, handle_set_steps, handle_vibrate, handle_sleep, };

void handle_main_menu() {
    handle_generic_menu(menu_labels, menu_callbacks);
}
