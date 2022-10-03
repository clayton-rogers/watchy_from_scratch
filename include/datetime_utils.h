#pragma once

#include <TimeLib.h>

// After any member has been incremented or decremented, this fn can be called
// to clamp all the of members to the proper bounds.
void normalize_datetime(tmElements_t* tm);

tmElements_t add_datetime(const tmElements_t& a, const tmElements_t& b);
tmElements_t sub_datetime(const tmElements_t& a, const tmElements_t& b);

int time_delta_minutes(const tmElements_t& from, const tmElements_t& to);
