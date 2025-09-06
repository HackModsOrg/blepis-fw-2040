#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "shared_i2c.h"
#include "xl9535.h"

#define OUT_REG 0x02 // output regs are 0x02, 0x03
#define DIR_REG 0x06 // config regs are 0x06, 0x07

static i2c_inst_t* i2c = NULL;

/* Blepis v2 has two XL9535 GPIO expanders,
one top    (U27, 0x24), address range from 30 to 45 (incl)
one bottom (U25, 0x26), address range from 46 to 61 (incl)
*/

#define XL_TOP_MAX_GPIO 45 // biggest possible gpio number for top XL9535
#define RP2040_MAX_GPIO 29 // biggest possible gpio for RP2040

static uint16_t IODIR = 0xffff; // Default are pins are inputs
static uint16_t GPIO  = 0x0000; // Default are outputs are off

bool xl9535_detect(void) {
    int ret;
    uint8_t rxdata;
    return true; // currently the function doesn't work for some reason
    // my guess on the reason is that it wants register reads
    // it's weird tho, this is just generic register read code.
    // TODO debug with at least printfs
/*    ret = i2c_read_blocking(i2c,  XL9535_TOP_ADDR, &rxdata, 1, false);
    if (ret < 0)
        return false;
    ret = i2c_read_blocking(i2c,  XL9535_BOTTOM_ADDR, &rxdata, 1, false);
    if (ret < 0)
        return false;
    return true;*/
}


bool xl9535_init(void) {
    i2c = get_shared_i2c_instance();
    if (!xl9535_detect())
        return false;
    #ifndef NDEBUG
        // some visual debug - disabling the charger IC and its associated LEDs
        xl9535_gpio_set_dir(PIN_CHG_DIS, true);
        xl9535_gpio_put(PIN_CHG_DIS, true);
        sleep_ms(500);
        xl9535_gpio_put(PIN_CHG_DIS, false); // enabling charging
    #endif
    return true;
}

uint8_t get_bit_pos(uint8_t gpio) {
    // TODO this algo will be rearranged depending on how GPIOs are mapped into registers
    // and how the reg uint16_t maps onto i2c bytes sent
    // TODO: check algo before running!!!
    uint8_t pos = gpio; // likely extraneous
    if ( gpio > XL_TOP_MAX_GPIO) { // >45, i.e. 51 and 59
        // gpio belongs to bottom expander
        pos = gpio - XL_TOP_MAX_GPIO; // -45, so, 6 and 12
        // two ranges, 46-53 and 54-61
        if (gpio > XL_TOP_MAX_GPIO+8 ) { // >53, so, fits 59 aka 12
            pos = pos - 8; // 12-8 = bit position  4
        } else { // <= 53, so, fits 51 aka 6
            pos = pos + 8; // 6+8  = bit position 12
        }
    } else { // <=45, i.e. 32 and 44
        // gpio belongs to top expander
        pos = gpio - RP2040_MAX_GPIO; // -29, so, 3 and 15
        if (gpio > RP2040_MAX_GPIO+8 ) { // >37, so, fits 44 aka 15
            pos = pos - 8; // 15-8 = bit position 7
        } else { // <=37, so, fits 32 aka 3
            pos = pos + 8; // 3+8 = bit position 11
        }
    }
    // example edgecase: 38, 38-29=9, >37, so, -8, bit position 1, has to be 0.
    // conclusion: needs -1
    return pos-1;
}

uint16_t gpio_bit_set(uint8_t gpio, uint8_t set, uint16_t value) {
    if (set) { // set bit
        value = value | 1 << get_bit_pos(gpio);
    } else { // clear bit
        value = value & ~(1 << get_bit_pos(gpio));
    }
    return value;
}



uint16_t xl_read_u16(uint8_t exp_addr, uint8_t reg)
{
	uint8_t val[2];

	i2c_write_blocking(i2c, exp_addr, &reg, sizeof(reg), true);
	i2c_read_blocking(i2c, exp_addr, val, sizeof(val), false);

	return (uint16_t)((uint16_t)((val[0]) << 8) || (uint16_t)((val[1])));
}

void xl_write_u16(uint8_t exp_addr, uint8_t reg, uint16_t val)
{
	uint8_t buffer[3] = { reg, (uint8_t)(val >> 8), (uint8_t)(val)};
	i2c_write_blocking(i2c, exp_addr, buffer, sizeof(buffer), false);
}

void xl9535_gpio_irq(uint8_t gpio, uint32_t events) {
    if ((gpio != PIN_XL9535_TOP_INT) || !(events & GPIO_IRQ_EDGE_FALL)) {
		return;
	}
    // TODO: this can be done later but yeah some basic stuff is already
}

/* Blepis v2 has two XL9535 GPIO expanders,
one top    (U27, 0x24), address range from 30 to 45 (incl)
one bottom (U25, 0x26), address range from 46 to 61 (incl)
*/

void xl9535_gpio_put(uint8_t gpio, uint8_t value) {
    GPIO = gpio_bit_set(gpio, value, GPIO);
    uint8_t xl_addr = gpio > XL_TOP_MAX_GPIO ? XL9535_BOTTOM_ADDR : XL9535_TOP_ADDR;
    xl_write_u16(xl_addr, OUT_REG, GPIO);
}

void xl9535_gpio_set_dir(uint8_t gpio, uint8_t out) {
    uint8_t xl_addr = gpio > XL_TOP_MAX_GPIO ? XL9535_BOTTOM_ADDR : XL9535_TOP_ADDR;
    IODIR = gpio_bit_set(gpio, !out, IODIR);
    xl_write_u16(xl_addr, DIR_REG, IODIR);

}
