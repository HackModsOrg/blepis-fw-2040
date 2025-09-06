#include "app_config.h"
#include "rtc.h"

#include <stdio.h>

#include <pico/stdlib.h>
#include <pico/util/datetime.h>
#include <RP2040.h>
#include <hardware/rtc.h>

// https://electronics.stackexchange.com/questions/66285/how-to-calculate-day-of-the-week-for-rtc
static int leap(int year)
{
	return year*365 + (year/4) - (year/100) + (year/400);
}
static int zeller(int year, int month, int day)
{
	year += ((month+9)/12) - 1;
	month = (month+9) % 12;
	return leap (year) + month*30 + ((6*month+5)/10) + day + 1;
}
static int dow(int year, int month, int day)
{
	return (zeller (year, month, day) % 7);
}

void rtc_set(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	datetime_t t;
	t.year = year + 1900;
	t.month = month;
	t.day = day;
	t.hour = hour;
	t.min = min;
	t.sec = sec;
	t.dotw = dow(t.year, month, day);

	bool ret = rtc_set_datetime(&t);
    if (!ret) {
        printf("Failed to set rtc!");
    }
}

uint8_t rtc_get(enum reg_id reg)
{
	datetime_t t;

	rtc_get_datetime(&t);

	switch (reg) {
		case REG_ID_RTC_SEC: return (uint8_t)t.sec;
		case REG_ID_RTC_MIN: return (uint8_t)t.min;
		case REG_ID_RTC_HOUR: return (uint8_t)t.hour;
		case REG_ID_RTC_MDAY: return (uint8_t)t.day;
		case REG_ID_RTC_MON: return (uint8_t)t.month;
		case REG_ID_RTC_YEAR: return (uint8_t)(t.year - 1900);
	}

	return 0;
}
