#pragma once

#include <Arduino.h>


struct WeatherData {
    int8_t temperature;
    int16_t weather_condition_code;
};

// Returns true on success
bool update_weather_data_from_internet();

WeatherData get_weather_data();
