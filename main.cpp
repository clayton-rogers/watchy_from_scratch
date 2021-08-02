#include <Arduino.h>

//pins
#define SDA 21
#define SCL 22
#define ADC_PIN 33
#define RTC_PIN GPIO_NUM_27
#define CS 5
#define DC 10
#define RESET 9
#define BUSY 19
#define VIB_MOTOR_PIN 13
#define MENU_BTN_PIN 26
#define BACK_BTN_PIN 25
#define UP_BTN_PIN 32
#define DOWN_BTN_PIN 4
#define MENU_BTN_MASK GPIO_SEL_26
#define BACK_BTN_MASK GPIO_SEL_25
#define UP_BTN_MASK GPIO_SEL_32
#define DOWN_BTN_MASK GPIO_SEL_4
#define ACC_INT_MASK GPIO_SEL_14
#define BTN_PIN_MASK MENU_BTN_MASK|BACK_BTN_MASK|UP_BTN_MASK|DOWN_BTN_MASK
//display
#define DISPLAY_WIDTH 200
#define DISPLAY_HEIGHT 200




#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>



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

static void display_screen() {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(0,18);
    display.print("Hello ");
    display.print(counter++);

    display.setCursor(0,2*18);
    display.print(init_time);
    display.setCursor(0,3*18);
    display.print(last_total_time);


    display.display(first_time ? false : true);
}

void setup(){
    init_time = micros();
    watch_init();
    display_screen();

    first_time = false;
    last_total_time = micros();
    deep_sleep();
}

void loop(){}
