#include "calendar.h"

#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include "API_KEYS.h"

#include "clock.h"
#include "datetime_utils.h"

// Approx 27 * 10 = 270 bytes
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
        return;
    }
    //Serial.print("Post for new access token: ");
    //Serial.println(http_response_code);
    JSONVar response = JSON.parse(http.getString());
    //Serial.println(response);
    //Serial.println(response.length());
    //Serial.println(response.keys());
    //Serial.println(response.hasOwnProperty("access_token"));
    //Serial.println(JSONVar::typeof_(response));
    //Serial.println(JSONVar::typeof_(response["access_token"]));
    //Serial.println(JSONVar::stringify(response));
    access_token = (const char*)response["access_token"];
    //Serial.println(access_token);
    //Serial.println();

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
    http.setConnectTimeout(5000);//3sec

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

    //Serial.println();
    //Serial.println();
    //Serial.println("update_calendar()");
    //Serial.println(query_url);

    http.begin(query_url);
    String auth = String("Bearer ") + access_token;
    //Serial.print("Auth str: ");
    //Serial.println(auth);
    //Serial.println(String("Bearer "));
    //Serial.println(access_token);
    //Serial.println(String("Bearer ") + access_token);
    http.addHeader("Authorization", auth);
    int http_response_code = http.GET();
    //Serial.println();
    //Serial.println(String("Response code: ") + String(http_response_code));
    if (http_response_code == 200) {
        JSONVar response = JSON.parse(http.getString());
        JSONVar items = response["items"];
        //Serial.println(items.length());
        for (int i = 0; i < 10; ++i) {
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
        Serial.println("bad HTTP response code");
        // Print error?
    }
    http.end();
}

void update_calendar_from_internet() {
    Serial.begin(115200);

    refresh_access_token();
    update_calendar();
}

calendar_event_t get_next_calendar_event() {
//    tmElements_t current_time = get_date_time();
//
//    // Sub 10 mins, so if the event has just started, then we still show it
//
//
//    for (int i = 0; i < CALENDAR_SIZE; ++i) {
//        // Find the first event that is in the future
//    }
//
    return calendar[0];
}
