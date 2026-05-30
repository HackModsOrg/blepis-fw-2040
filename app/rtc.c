#include "app_config.h"
#include "rtc.h"
#include "gpio.h"
#include "shared_i2c.h"

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

bool disable_i2c_rtc_clk() {
    printf("1\r\n");
    i2c_inst_t* puppet_host_i2c = get_puppet_shared_i2c_instance();
    i2c_scan(puppet_host_i2c);
    //uni_gpio_put(PIN_IO_MUX_SEL, 1); // disconnect top I2C expanders; works
    bool success = false;
    uint8_t buffer[2] = { 0x27, 0x8f };
    int ret;
        uni_gpio_put(PIN_I2C_PU_PWR, 1);
        uni_gpio_set_dir(PIN_I2C_PU_PWR, GPIO_OUT);
    sleep_ms(200);
    for (int i=0;i<3;i++) {
        i2c_scan(puppet_host_i2c);
        printf("2\r\n");
        printf("3\r\n");
        ret = i2c_write_timeout_us(puppet_host_i2c, 0x51, buffer, sizeof(buffer), false, 100000);
        printf("4\r\n");
        printf("rtc shutup %d \r\n", ret);
        if (ret > 0) {
            uni_gpio_put(PIN_IO_MUX_SEL, 0); // reconnect top I2C
            return true;
        }
        sleep_ms(100*(i+1));
    }
    //uni_gpio_put(PIN_IO_MUX_SEL, 0); // reconnect top I2C
    i2c_scan(puppet_host_i2c);
    if (ret < 0) {
        return false;
    }
    return true;
}
