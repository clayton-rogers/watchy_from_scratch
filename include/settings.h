#pragma once

struct settings_t {
    bool internet_update = true;
    int reset_hour = 23;
    int reset_minute = 50;
    int sleep_hour = 01;
    int sleep_minute = 00;
};

settings_t get_settings();

void handle_settings_menu();
