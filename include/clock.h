#pragma once

#include <TimeLib.h>

tmElements_t get_date_time();

void set_date_time(const tmElements_t& new_time);

void first_time_rtc_config();

void clock_init();

void disable_minute_alarm();
