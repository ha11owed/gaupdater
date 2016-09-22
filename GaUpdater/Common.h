#pragma once

#include <time.h>

#define DATETIME_FMT "yyyy-MM-dd HH:mm:ss"

void DateTimeParseGeneric(const char* fmt, const char* str, tm& outTm);

time_t StrToTimeUTC(const char* fmt, const char* str);

time_t StrToTime(const char* fmt, const char* str);
