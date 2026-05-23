#include "reg.h"

void rtc_set(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

uint8_t rtc_get(enum reg_id reg);

bool disable_i2c_rtc_clk();
