#include "button.h"

#include <Arduino.h>
#include "pin_def.h"

static bool first = true;

// TODO handle button presses in interrupt

Button get_next_button() {
    if (first) {
        uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();

        // Initially button press state is populated by the wakeup
        Button b;
        if (wakeupBit & MENU_BTN_MASK) b = Button::MENU;
        if (wakeupBit & BACK_BTN_MASK) b = Button::BACK;
        if (wakeupBit & UP_BTN_MASK)   b = Button::UP;
        if (wakeupBit & DOWN_BTN_MASK) b = Button::DOWN;

        first = false;
        return b;
    }
    while (1) {
        if (digitalRead(MENU_BTN_PIN)) return Button::MENU;
        if (digitalRead(BACK_BTN_PIN)) return Button::BACK;
        if (digitalRead(UP_BTN_PIN))   return Button::UP;
        if (digitalRead(DOWN_BTN_PIN)) return Button::DOWN;
    }
}
