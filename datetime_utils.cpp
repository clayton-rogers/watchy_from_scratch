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
