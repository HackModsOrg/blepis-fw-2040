#include "shared_i2c.h"
#include "gpio.h"

#include <hardware/i2c.h>
#include <stdio.h>
#include <pico/stdlib.h>

void wusb3801_init(uint8_t addr) {
    i2c_inst_t* i2c = get_shared_i2c_instance();
    // CTRL0 (0x02):
    // no acc (7, 1), try SRC (6-5, 10), default current (4-3, 00), DRP (2-1, 10), ~~1~~ 0
    // == 0b1 10 00 10 0 == 0xc4
    uint8_t buffer[2] = { 0x02, 0xc2 };
    int ret = i2c_write_timeout_us(i2c, addr, buffer, sizeof(buffer), false, 10000);
    if (ret < 0) {
        printf("failed WUSB3801 init step 1 addr %X", addr);
        return;
    }
}

void usbc_init() {
    wusb3801_init(WUSB_RIGHT_ADDR);
    wusb3801_init(WUSB_LEFT_ADDR);
}

void usbc_debug() {

}


