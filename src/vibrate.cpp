#include "vibrate.h"

#include <Arduino.h>

#include "pin_def.h"

void vibrate() {
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    bool motorOn = false;
    int length = 10;
    uint32_t interval_ms = 100;
    for (int i = 0; i < length; ++i) {
        motorOn = !motorOn;
        digitalWrite(VIB_MOTOR_PIN, motorOn);
        delay(interval_ms);
    }
}
