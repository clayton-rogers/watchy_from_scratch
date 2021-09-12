#include "calendar.h"

#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include "API_KEYS.h"

#include "clock.h"
#include "datetime_utils.h"

// Approx 47 * 10 = 470 bytes
static const int CALENDAR_SIZE = 10;
RTC_DATA_ATTR calendar_event_t calendar[CALENDAR_SIZE];



#define GOOGLE_CALENDAR_EVENTS_URL "https://www.googleapis.com/calendar/v3/calendars/primary/events"
#define GOOGLE_CALENDAR_EVENTS_PARAMS "?orderBy=startTime&singleEvents=true"


static String access_token;

static void refresh_access_token() {

    HTTPClient http;
    http.setConnectTimeout(3000);//3sec

    String query_url =
        String("https://oauth2.googleapis.com/token");
    http.begin(query_url);

    String payload =
        String("client_secret=") + String(GOOGLE_CALENDAR_CLIENT_SECRET) +
        String("&grant_type=refresh_token") +
        String("&refresh_token=") + String(GOOGLE_CALENDAR_REFRESH_TOKEN) +
        String("&client_id=") + String(GOOGLE_CALENDAR_CLIENT_ID);
    http.addHeader("content-type", "application/x-www-form-urlencoded");
    int http_response_code = http.POST(payload);
    if (http_response_code != 200) {
        Serial.println("Bad HTTP response code to refresh access token");
        Serial.println(http.getString());
        http.end();
        return;
    }

    JSONVar response = JSON.parse(http.getString());
    access_token = (const char*)response["access_token"];

    http.end();
}

static String datetime_to_string(const tmElements_t& time) {
    String result;

    auto pad_int_to_string = [](int number) -> String {
        if (number < 10) {
            return String("0") + String(number);
        } else {
            return String(number);
        }
    };

    int year = 1970 + time.Year;
    result += pad_int_to_string(year);
    result += "-";

    result += pad_int_to_string(time.Month);
    result += "-";

    result += pad_int_to_string(time.Day);
    result += "T";

    result += pad_int_to_string(time.Hour);
    result += "%3A"; // ':'

    result += pad_int_to_string(time.Minute);
    result += "%3A"; // ':'

    result += pad_int_to_string(time.Second);
    result += "-04%3A00"; // timezone

    return result;
}

static tmElements_t string_to_datetime(const String& input) {
    tmElements_t t = {};

    // ex:
    // 2021-08-10T15:00:00-04:00
    // 012345678901234567890123456
    String temp;

    temp = input.substring(0, 4);
    t.Year = temp.toInt() - 1970;

    temp = input.substring(5, 7);
    t.Month = temp.toInt();

    temp = input.substring(8, 10);
    t.Day = temp.toInt();

    temp = input.substring(11, 13);
    t.Hour = temp.toInt();

    temp = input.substring(14, 16);
    t.Minute = temp.toInt();

    temp = input.substring(17, 19);
    t.Second = temp.toInt();

    return t;
}

static void update_calendar() {
    HTTPClient http;
    http.setConnectTimeout(5000);//5sec

    // Sub 10 mins, so if the event has just started, then we still show it
    tmElements_t current_time = get_date_time();

    tmElements_t ten_mins = {};
    ten_mins.Month = 1;
    ten_mins.Day = 1;
    ten_mins.Minute = 10;
    tmElements_t query_time = sub_datetime(current_time, ten_mins);

    String query_url =
        String(GOOGLE_CALENDAR_EVENTS_URL) +
        String(GOOGLE_CALENDAR_EVENTS_PARAMS) +
        String("&maxResults=") + String(CALENDAR_SIZE) +
        String("&timeMin=") +  datetime_to_string(query_time) + //String("2021-08-08T23%3A00%3A00-07%3A00") +
        String("&key=") + String(GOOGLE_CALENDAR_API_KEY);

    http.begin(query_url);
    String auth = String("Bearer ") + access_token;
    http.addHeader("Authorization", auth);
    int http_response_code = http.GET();
    if (http_response_code == 200) {
        JSONVar response = JSON.parse(http.getString());
        JSONVar items = response["items"];
        for (int i = 0; i < CALENDAR_SIZE; ++i) {
            JSONVar event = items[i];
            String event_name = (const char*)event["summary"];
            String event_start = (const char*)event["start"]["dateTime"];
            Serial.println(event_name + String(" ") + event_start);
            tmElements_t datetime = string_to_datetime(event_start);
            strncpy(calendar[i].name, event_name.c_str(), CALENDAR_EVENT_NAME_LENGTH);
            calendar[i].name[CALENDAR_EVENT_NAME_LENGTH-1] = '\0';
            calendar[i].start_time = datetime;
        }
    } else {
        Serial.println("Bad HTTP response code to calendar query");
        Serial.println(http.getString());
    }
    http.end();
}

void update_calendar_from_internet() {
    refresh_access_token();
    update_calendar();
}

calendar_event_t get_next_calendar_event() {

    tmElements_t current_time = get_date_time();
    tmElements_t ten_mins = {};
    ten_mins.Month = 1;
    ten_mins.Day = 1;
    ten_mins.Minute = 10;
    tmElements_t query_time = sub_datetime(current_time, ten_mins);


    for (int i = 0; i < CALENDAR_SIZE; ++i) {
        int time_till_event = time_delta_minutes(query_time, calendar[i].start_time);
        if (time_till_event > 0) {
            return calendar[i];
        }
    }

    // if we don't have a event that's in the future, then return a null one
    calendar_event_t null_event = {};
    null_event.start_time.Month = 1; // Just to make the date valid
    null_event.start_time.Day = 1;
    return null_event;
}
