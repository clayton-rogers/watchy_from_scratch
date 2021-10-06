#include "generic_set_time.h"

#include "watchy_display.h"
#include <DSEG7_Classic_Bold_53.h> // Time
#include "button.h"


static void draw_time(const int* hour, const int* minute, int digit_select) {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&DSEG7_Classic_Bold_53);

    display.setCursor(5, 80);
    display.printf("%02d:%02d", *hour, *minute);

    const int y = 100;
    const int x_offsets[]  = { 25, 75, 125, 175 };
    const int x = x_offsets[digit_select];
    display.fillTriangle(x, y, x - 10, y + 10, x + 10, y + 10, GxEPD_WHITE);

    display.display(true);
}

static void normalize_time(int& hour, int& minute) {
    if (minute < 0) {
        minute += 60;
        hour -= 1;
    }
    if (minute >= 60) {
        minute -= 60;
        hour += 1;
    }
    if (hour < 0) {
        hour = 23;
    }
    if (hour >= 24) {
        hour = 0;
    }
}

void generic_set_time(int* hour, int* minute) {
    int digit_sel = 0;
    // 0 = 10 hours
    // 1 =  1 hours
    // 2 = 10 minutes
    // 3 =  1 minutes
    Button b = Button::NONE;
    while (1) {
        if (b == Button::MENU) {
            digit_sel++;
            if (digit_sel > 3) digit_sel = 0;
        }
        if (b == Button::UP) {
            switch (digit_sel) {
                case 0:
                    *hour += 10; break;
                case 1:
                    *hour +=  1; break;
                case 2:
                    *minute += 10; break;
                case 3:
                    *minute +=  1; break;
            }
        }
        if (b == Button::DOWN) {
            switch (digit_sel) {
                case 0:
                    *hour -= 10; break;
                case 1:
                    *hour -=  1; break;
                case 2:
                    *minute -= 10; break;
                case 3:
                    *minute -=  1; break;
            }
        }
        if (b == Button::BACK) break;

        normalize_time(*hour, *minute);

        draw_time(hour, minute, digit_sel);
        b = get_next_button();
    }
}
