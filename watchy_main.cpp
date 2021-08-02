#include "watchy_main.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include "pin_def.h"

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY));
RTC_DATA_ATTR bool first_time = true;
RTC_DATA_ATTR int counter = 0;
RTC_DATA_ATTR unsigned long last_total_time = 0;

static unsigned long init_time;

static void watch_init() {
    display.init(0, false);
    display.setFullWindow();
}

static void deep_sleep() {
    display.hibernate();
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
    esp_deep_sleep_start();
}

static void display_screen(int input) {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(0,18);
    display.print("Hello ");
    display.println(counter++);
    display.println(init_time);
    display.println(last_total_time);
    display.println(input);


    display.display(first_time ? false : true);
}

static void display_watchface(bool partial_refresh) {
    
}

volatile static int gate = 1;
volatile static int output = 0;
void task(void * pvParameters) {
    while (gate) {
        output = 1;
    }
}

void run_watch() {
    init_time = micros();
    watch_init();
    display_screen(0);

    TaskHandle_t xHandle = NULL;
    BaseType_t xReturned = xTaskCreate(
        task,
        "second",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES,
        &xHandle);

    if (xReturned == pdPASS) {
        gate = 1;
        while (!output) {
            NOP();
        }
        vTaskDelete(xHandle);
    }

    display_screen(1);

    first_time = false;
    last_total_time = micros();
    deep_sleep();
}
