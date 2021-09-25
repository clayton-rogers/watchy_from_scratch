#include "settings.h"

#include <Arduino.h>

#include "watchy_display.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include "generic_menu.h"
#include "button.h"


RTC_DATA_ATTR settings_t settings;

settings_t get_settings() {
    return settings;
}

static void draw_internet_update() {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(0, 30);
    display.print("Internet Update:");
    display.setCursor(0, 60);
    if (settings.internet_update) {
        display.print("ON");
    } else {
        display.print("OFF");
    }

    const bool partial_refresh = true;
    display.display(partial_refresh);
}

static void handle_internet_update() {
    Button b = Button::NONE;
    while (1) {
        if (b == Button::MENU ||
            b == Button::UP ||
            b == Button::DOWN) {

            settings.internet_update = !settings.internet_update;
        }
        if (b == Button::BACK) break;

        draw_internet_update();
        b = get_next_button();
    }
}

static void draw_reset_time(bool select_hour) {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(0, 30);
    display.print("Steps Reset:");
    display.setCursor(0, 60);
    display.print("Time that steps");
    display.setCursor(0, 90);
    display.print("reset");
    display.setCursor(0, 120);
    display.printf("   %02d:%02d", settings.reset_hour, settings.reset_minute);

    // Draw hour/minute index
    {
        const int x = select_hour ? 45 : 75;
        const int y = 130;

        display.fillTriangle(x, y, x - 8, y + 8, x + 8, y + 8, GxEPD_WHITE);
    }

    const bool partial_refresh = true;
    display.display(partial_refresh);
}

static void handle_reset_time() {
    bool select_hour = true;
    Button b = Button::NONE;
    while (1) {
        if (b == Button::MENU) select_hour = !select_hour;
        if (b == Button::UP) {
            if (select_hour) {
                ++settings.reset_hour;
                if (settings.reset_hour == 24) settings.reset_hour = 0;
            } else {
                ++settings.reset_minute;
                if (settings.reset_minute == 60) settings.reset_minute = 0;
            }
        }
        if (b == Button::DOWN) {
            if (select_hour) {
                --settings.reset_hour;
                if (settings.reset_hour == -1) settings.reset_hour = 23;
            } else {
                --settings.reset_minute;
                if (settings.reset_minute == -1) settings.reset_minute = 59;
            }
        }
        if (b == Button::BACK) break;

        draw_reset_time(select_hour);
        b = get_next_button();
    }
}

static const char* menu_labels[] =
    {"Internet Update", "Steps Reset", "====", "====", "====", "===="};
static menu_handler_ptr menu_callbacks[] =
    {handle_internet_update, handle_reset_time, null_menu, null_menu, null_menu, null_menu };

void handle_settings_menu() {
    handle_generic_menu(menu_labels, menu_callbacks);
}
