#include "battery.h"

#include <Arduino.h>

#include "pin_def.h"

RTC_DATA_ATTR float bat_adc_offset = -0.07f;

float get_battery_voltage() {
    // Battery voltage goes through a 1/2 divider.
    return analogReadMilliVolts(ADC_PIN) / 1000.0f * 2.0f + bat_adc_offset;
}

float get_battery_percentage() {
    float voltage = get_battery_voltage();
    float percentage = 2808.3808f * powf(voltage, 4)
                        - 43560.9157f * powf(voltage, 3)
                        + 252848.5888f * powf(voltage, 2)
                        - 650767.4615f * voltage
                        + 626532.5703f;
    percentage = max(0.0f, percentage);
    percentage = min(100.0f, percentage);
    return percentage;
}

float get_adc_cal() {
    return bat_adc_offset;
}

void  set_adc_cal(float value) {
    bat_adc_offset = value;
}
