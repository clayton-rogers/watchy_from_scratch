#include "generic_menu.h"

#include "watchy_display.h"
#include <Fonts/FreeMonoBold9pt7b.h> // Menu
#include "button.h"

#define MENU_HEIGHT 30
#define MENU_LENGTH 6
static void draw_menu(int menu_index, bool partial_refresh, const char** menu_labels) {
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t  x1, y1;
    uint16_t w, h;
    int16_t yPos;

    for (int i = 0; i < MENU_LENGTH; i++){
        yPos = 30+(MENU_HEIGHT*i);
        display.setCursor(0, yPos);
        if (i == menu_index) {
            display.getTextBounds(menu_labels[i], 0, yPos, &x1, &y1, &w, &h);
            display.fillRect(x1-1, y1-10, 200, h+15, GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            display.println(menu_labels[i]);
        } else {
            display.setTextColor(GxEPD_WHITE);
            display.println(menu_labels[i]);
        }
    }

    display.display(partial_refresh);
}

void handle_generic_menu(const char** menu_labels, menu_handler_ptr* menu_callbacks) {

    int menu_index = 0;
    Button b = Button::NONE;
    while (1) {
        if (b == Button::MENU) menu_callbacks[menu_index]();
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
        draw_menu(menu_index, true, menu_labels);
        b = get_next_button();
    }
}

void null_menu() {}
