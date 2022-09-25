#include "weather.h"


#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include "API_KEYS.h"


//weather api
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?q="
#define TEMP_UNIT "metric" //use "imperial" for Fahrenheit"


RTC_DATA_ATTR WeatherData currentWeather;

bool update_weather_data_from_internet(){
    HTTPClient http;
    bool success = false;
    http.setConnectTimeout(3000);//3 second max timeout
    String weatherQueryURL = String(OPENWEATHERMAP_URL) + String(CITY_ID) + String("&units=") + String(TEMP_UNIT) + String("&appid=") + String(OPENWEATHERMAP_APIKEY);
    http.begin(weatherQueryURL.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp"]);
        currentWeather.weather_condition_code = int(responseObject["weather"][0]["id"]);
        success = true;
    } else {
        //http error
        currentWeather.temperature = 0;
        currentWeather.weather_condition_code = 800;
        success = false;
    }
    http.end();

    return success;
}

WeatherData get_weather_data() {
    return currentWeather;
}
