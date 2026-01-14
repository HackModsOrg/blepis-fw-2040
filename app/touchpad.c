#include "touchpad.h"
#include "gpio.h"
#include "keyboard.h"

#include <hardware/i2c.h>
#include <pico/binary_info.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "reg.h"
#include "shared_i2c.h"

#define DEV_ADDR			0x3B

#define REG_PID				0x00
#define REG_REV				0x01

#define REG_MOTION			0x02
#define BIT_MOTION_MOT		(1 << 7)
#define BIT_MOTION_OVF		(1 << 4)

#define REG_DELTA_X			0x03
#define REG_DELTA_Y			0x04

// Surface quality
#define REG_SQUAL			0x05

#define REG_CONFIG			0x11
#define BIT_CONFIG_HIRES	(1 << 7)

#define REG_LED				0x1A
#define LED_MED				0x00
#define LED_HIGH			0x03
#define LED_LOW				0x05

#define REG_OBSERV			0x2E
#define BIT_OBSERV_RUN		(0 << 6)
#define BIT_OBSERV_REST1	(1 << 6)
#define BIT_OBSERV_REST2	(2 << 6)
#define BIT_OBSERV_REST3	(3 << 6)

#define REG_ORIENTATION		0x77
#define BIT_ORIENTATION_X_INV (1 << 5)
#define BIT_ORIENTATION_Y_INV (1 << 6)


static struct
{
	struct touch_callback *callbacks;
	i2c_inst_t *i2c;
} self;

bool touchpad_detect()
{
    int ret;
    uint8_t rxdata;
    ret = i2c_read_blocking(self.i2c, DEV_ADDR, &rxdata, 1, false);
    return ret >= 0;
}

uint8_t touchpad_read_i2c_u8(uint8_t reg)
{
	uint8_t val;

	i2c_write_blocking(self.i2c, DEV_ADDR, &reg, sizeof(reg), true);
	i2c_read_blocking(self.i2c, DEV_ADDR, &val, sizeof(val), false);

	return val;
}

void touchpad_write_i2c_u8(uint8_t reg, uint8_t val)
{
	uint8_t buffer[2] = { reg, val };
	i2c_write_blocking(self.i2c, DEV_ADDR, buffer, sizeof(buffer), false);
}

int64_t release_key(alarm_id_t id, void *user_data)
{
	(void)id;

	const int data = (int)user_data;

	keyboard_inject_event((char)data, KEY_STATE_RELEASED);

	return 0;
}

void touchpad_gpio_irq(uint gpio, uint32_t events)
{
	uint8_t reg;
	int8_t x, y;

	if ((gpio != PIN_TP_MOTION) || !(events & GPIO_IRQ_EDGE_FALL)) {
		return;
	}

	reg = touchpad_read_i2c_u8(REG_MOTION);

	// Overflow, clear registers
	if (reg & BIT_MOTION_OVF) {
		(void)touchpad_read_i2c_u8(REG_DELTA_X);
		(void)touchpad_read_i2c_u8(REG_DELTA_Y);
		return;
	}

	if (reg & BIT_MOTION_MOT) {

		// Get touchpad coordinates, clear registers
		x = touchpad_read_i2c_u8(REG_DELTA_X);
		y = touchpad_read_i2c_u8(REG_DELTA_Y);

		// Reject if surface quality is below threshold
		if (touchpad_read_i2c_u8(REG_SQUAL)
		  < reg_get_value_quiet(REG_ID_TOUCHPAD_MIN_SQUAL)) {
			return;
		}

		if (self.callbacks) {
			struct touch_callback *cb = self.callbacks;

			while (cb) {
				cb->func(x, y);

				cb = cb->next;
			}
		}
	}
}

void touchpad_add_touch_callback(struct touch_callback *callback)
{
	// first callback
	if (!self.callbacks) {
		self.callbacks = callback;
		return;
	}

	// find last and insert after
	struct touch_callback *cb = self.callbacks;
	while (cb->next)
		cb = cb->next;

	cb->next = callback;
}


bool touchpad_init()
{
	uint8_t val;

	// determine the instance based on SCL pin, hope you didn't screw up the SDA pin!
	self.i2c = get_shared_i2c_instance();

	uni_gpio_init(PIN_TP_SHUTDOWN);
	uni_gpio_set_dir(PIN_TP_SHUTDOWN, GPIO_OUT);
	uni_gpio_put(PIN_TP_SHUTDOWN, 0);

	gpio_init(PIN_TP_MOTION);
	gpio_set_dir(PIN_TP_MOTION, GPIO_IN);
	gpio_set_irq_enabled(PIN_TP_MOTION, GPIO_IRQ_EDGE_FALL, true);

	uni_gpio_init(PIN_TP_RESET);
	uni_gpio_set_dir(PIN_TP_RESET, GPIO_OUT);

	uni_gpio_put(PIN_TP_RESET, 0);
	sleep_ms(100);
	uni_gpio_put(PIN_TP_RESET, 1);

	sleep_ms(100); // vibes: letting the touchpad spend a lil time out of reset

    if (!touchpad_detect())
        return false;

	// Invert X motion
	val = touchpad_read_i2c_u8(REG_ORIENTATION);
	val |= BIT_ORIENTATION_X_INV;
	touchpad_write_i2c_u8(REG_ORIENTATION, val);

	// Set power to high
	touchpad_set_led_power(LED_HIGH);
    return true;
}

void touchpad_set_led_power(uint8_t setting)
{
	uint8_t val;

	// Set lower 3 bits
	val = touchpad_read_i2c_u8(REG_LED);
	val = (val & ~0x7) | setting;
	touchpad_write_i2c_u8(REG_LED, val);
}
