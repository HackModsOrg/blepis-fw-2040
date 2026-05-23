#include "shared_i2c.h"
#include "gpio.h"

#include <hardware/i2c.h>
#include <stdio.h>
#include <pico/stdlib.h>

void wusb3801_init(uint8_t addr) {
    i2c_inst_t* i2c = get_shared_i2c_instance();
    // 0xcb == 0b11001011 ==
    // no acc (7, 1), try SRC (6-5, 10), 1.5A (4-3, 01), SRC (2-1, 01), 1
    uint8_t buffer[2] = { 0x02, 0xcb };
    int ret = i2c_write_timeout_us(i2c, addr, buffer, sizeof(buffer), false, 10000);
    if (ret < 0) {
        printf("failed WUSB3801 init step 1 addr %X", addr);
        return;
    }
}

void usbc_init() {
    wusb3801_init(0x60);
    wusb3801_init(0x68);
}

void usbc_debug() {

}


