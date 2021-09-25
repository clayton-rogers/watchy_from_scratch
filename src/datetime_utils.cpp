#include "datetime_utils.h"

void normalize_datetime(tmElements_t* tm) {
    if (tm->Month < 1)     tm->Month = 12;
    if (tm->Month > 12)    tm->Month = 1;
    if (tm->Day < 1)       tm->Day = 31;
    if (tm->Day > 31)      tm->Day = 1;
    if (tm->Hour == 255)   tm->Hour = 23;
    if (tm->Hour > 23)     tm->Hour = 0;
    if (tm->Minute == 255) tm->Minute = 59;
    if (tm->Minute > 59)   tm->Minute = 0;
    if (tm->Second == 255) tm->Second = 59;
    if (tm->Second > 59)   tm->Second = 0;
}

tmElements_t add_datetime(const tmElements_t& a, const tmElements_t& b) {
    const time_t a_unix = makeTime(a);
    const time_t b_unix = makeTime(b);
    const time_t result_unix = a_unix + b_unix;

    tmElements_t result;
    breakTime(result_unix, result);

    return result;
}

tmElements_t sub_datetime(const tmElements_t& a, const tmElements_t& b) {
    const time_t a_unix = makeTime(a);
    const time_t b_unix = makeTime(b);
    const time_t result_unix = a_unix - b_unix;

    tmElements_t result;
    breakTime(result_unix, result);

    return result;
}

int time_delta_minutes(const tmElements_t& from, const tmElements_t& to) {

    const time_t a_unix = makeTime(from);
    const time_t b_unix = makeTime(to);
    const time_t result_seconds_unix = b_unix - a_unix;
    const time_t results_minutes_unix = result_seconds_unix / 60;

    return results_minutes_unix; // convert long to int
}
