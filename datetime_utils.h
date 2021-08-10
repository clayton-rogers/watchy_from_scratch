#pragma once

#include <TimeLib.h>


// After any member has been incremented or decremented, this fn can be called
// to clamp all the of members to the proper bounds.
void normalize_datetime(tmElements_t* tm);
