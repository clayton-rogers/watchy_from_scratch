#pragma once

#include <Arduino.h>
#include <TimeLib.h>


const int CALENDAR_EVENT_NAME_LENGTH = 20;
struct calendar_event_t {
    char name[CALENDAR_EVENT_NAME_LENGTH];
    tmElements_t start_time;
};

void update_calendar_from_internet();

calendar_event_t get_next_calendar_event();
