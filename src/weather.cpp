#include "weather.h"

#include <Arduino_JSON.h>
#include <HTTPClient.h>

#include "API_KEYS.h"

// weather api
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?"
#define TEMP_UNIT "metric"  // use "imperial" for Fahrenheit"

RTC_DATA_ATTR WeatherData currentWeather;
RTC_DATA_ATTR uint16_t fail_count = 0;

bool update_weather_data_from_internet() {
    HTTPClient http;
    bool success = false;
    http.setConnectTimeout(3000);  // 3 second max timeout
    String weatherQueryURL = String(OPENWEATHERMAP_URL) + String("id=") +
                             String(CITY_ID) + String("&units=") +
                             String(TEMP_UNIT) + String("&appid=") +
                             String(OPENWEATHERMAP_APIKEY);
    http.begin(weatherQueryURL.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp_max"]);
        currentWeather.weather_condition_code =
            int(responseObject["weather"][0]["id"]);
        success = true;
        fail_count = 0;
    } else {
        // http error
        success = false;
        ++fail_count;
        if (fail_count > 10) {
            currentWeather.temperature = 0;
            currentWeather.weather_condition_code = 800;
        }
    }
    http.end();

    return success;
}

WeatherData get_weather_data() { return currentWeather; }
