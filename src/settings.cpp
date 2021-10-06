#include "settings.h"

#include <Arduino.h>

#include "watchy_display.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include "generic_menu.h"
#include "button.h"
#include "generic_set_time.h"


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

static void handle_reset_time() {
    generic_set_time(&settings.reset_hour, &settings.reset_minute);
}

static const char* menu_labels[] =
    {"Internet Update", "Steps Reset", "====", "====", "====", "===="};
static menu_handler_ptr menu_callbacks[] =
    {handle_internet_update, handle_reset_time, null_menu, null_menu, null_menu, null_menu };

void handle_settings_menu() {
    handle_generic_menu(menu_labels, menu_callbacks);
}
