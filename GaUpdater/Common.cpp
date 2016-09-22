#include "stdafx.h"

time_t StrToTimeUTC(const char* fmt, const char* str)
{
	tm sTm;
	DateTimeParseGeneric(fmt, str, sTm);
	time_t t = _mkgmtime(&sTm);
	return t;
}

time_t StrToTime(const char* fmt, const char* str)
{
	tm sTm;
	DateTimeParseGeneric(fmt, str, sTm);
	time_t t = mktime(&sTm);
	return t;
}

void DateTimeParseGeneric(const char* fmt, const char* str, tm& outTm)
{
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;
	int second = 0;
	bool ok = true;

	char c, f, lastF = 0;
	for (size_t i = 0; (c = str[i]) != '\0' && (f = fmt[i]) != '\0' && ok; i++)
	{
		int d = -1;
		if (c >= '0' && c <= '9')
		{
			d = c - '0';
		}

		switch (f) {
		case 'y':
			if (d == -1) { ok = false; }
			else { year = 10 * year + d; }
			break;
		case 'M':
			if (d == -1) { ok = false; }
			else { month = 10 * month + d; }
			break;
		case 'd':
			if (d == -1) { ok = false; }
			else { day = 10 * day + d; }
			break;
		case 'H':
			if (d == -1) { ok = false; }
			else { hour = 10 * hour + d; }
			break;
		case 'm':
			if (d == -1) { ok = false; }
			else { minute = 10 * minute + d; }
			break;
		case 's':
			if (d == -1) { ok = false; }
			else { second = 10 * second + d; }
			break;
		default:
			if (d != -1) { ok = false; }
			break;
		};
	}

	outTm.tm_year = year - 1900;
	outTm.tm_mon = month - 1;
	outTm.tm_mday = day;

	outTm.tm_hour = hour;
	outTm.tm_min = minute;
	outTm.tm_sec = second;
}
