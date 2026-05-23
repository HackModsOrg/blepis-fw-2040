#include <hardware/i2c.h>
#include <pico/binary_info.h>
#include <pico/stdlib.h>
#include <stdio.h>

static i2c_inst_t* i2c_instances[2] = { i2c0, i2c1 };


void setup_shared_i2c(void) {
	i2c_inst_t* i2c = i2c_instances[(PIN_SCL / 2) % 2];

	i2c_init(i2c, 100 * 1000);

	gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SDA);

	gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SCL);

	// Make the I2C pins available to picotool
	bi_decl(bi_2pins_with_func(PIN_SDA, PIN_SCL, GPIO_FUNC_I2C));

}

void setup_puppet_i2c_as_shared_i2c(void) {
	i2c_inst_t* i2c = i2c_instances[(PIN_PUPPET_SCL / 2) % 2];

	i2c_init(i2c, 100 * 1000);

	gpio_set_function(PIN_PUPPET_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_PUPPET_SDA);

	gpio_set_function(PIN_PUPPET_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_PUPPET_SCL);

	// Make the I2C pins available to picotool
	bi_decl(bi_2pins_with_func(PIN_PUPPET_SDA, PIN_PUPPET_SCL, GPIO_FUNC_I2C));

}

i2c_inst_t* get_shared_i2c_instance(void) {
    return i2c_instances[(PIN_SCL / 2) % 2];
}

i2c_inst_t* get_puppet_shared_i2c_instance(void) {
    return i2c_instances[(PIN_PUPPET_SCL / 2) % 2];
}

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan(i2c_inst_t *i2c) {

    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_timeout_us(i2c, addr, &rxdata, 1, false, 10000);

        printf(ret < 0 ? (ret == -2 ? "t" : ".") : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}
