#include "reg.h"

#include "app_config.h"
#include "backlight.h"

#include "fifo.h"
#include "puppet_i2c.h"
#include "keyboard.h"
#include "touchpad.h"
#include "pi.h"
#include "hardware/adc.h"
#include "rtc.h"
#include "update.h"

#ifdef BLEPIS
#include "vibromotor.h"
#include "peripherals.h"
#endif

#ifdef BEEPY
#include "gpioexp.h"
#endif

#include <pico/stdlib.h>
#include <RP2040.h> // TODO: When there's more than one RP chip, change this to be more generic
#include <stdio.h>

// We don't enable this by default cause it spams quite a lot
//#define DEBUG_REGS

static struct
{
	uint8_t regs[REG_ID_LAST];
} self;

static void touch_cb(int8_t x, int8_t y)
{
	const int16_t dx = (int8_t)self.regs[REG_ID_TOX] + x;
	const int16_t dy = (int8_t)self.regs[REG_ID_TOY] + y;

	// bind to -128 to 127
	self.regs[REG_ID_TOX] = MAX(INT8_MIN, MIN(dx, INT8_MAX));
	self.regs[REG_ID_TOY] = MAX(INT8_MIN, MIN(dy, INT8_MAX));
}
static struct touch_callback touch_callback = { .func = touch_cb };

static int64_t update_commit_alarm_callback(alarm_id_t _, void* __)
{
	update_commit_and_reboot();
}

void reg_process_packet(uint8_t in_reg, uint8_t in_data, uint8_t *out_buffer, uint8_t *out_len)
{
	int rc;
	const bool is_write = (in_reg & PACKET_WRITE_MASK);
	const uint8_t reg = (in_reg & ~PACKET_WRITE_MASK);
	uint16_t adc_value;

    #ifndef NDEBUG
	printf("read complete, is_write: %d, reg: 0x%02X\r\n", is_write, reg);
    #endif

	*out_len = 0;

	switch (reg) {

	// common R/W registers
	case REG_ID_CFG:
	case REG_ID_INT:
	case REG_ID_DEB:
	case REG_ID_FRQ:
	case REG_ID_BKL:
	case REG_ID_BK2:
	case REG_ID_GIC:
	case REG_ID_GIN:
	case REG_ID_HLD:
	case REG_ID_ADR:
	case REG_ID_IND:
	case REG_ID_CF2:
	case REG_ID_SHUTDOWN_GRACE:
	case REG_ID_TOUCHPAD_MIN_SQUAL:
	case REG_ID_VBR:
	{
		if (is_write) {
			reg_set_value(reg, in_data);

			switch (reg) {
			case REG_ID_BKL:
			case REG_ID_BK2:
				backlight_sync();
				break;

            #ifdef BLEPIS
            case REG_ID_VBR:
                vibromotor_sync();
                break;
            #endif

			case REG_ID_ADR:
				puppet_i2c_sync_address();
				break;

			default:
				break;
			}
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	// special R/W registers
	case REG_ID_DIR: // gpio direction
	case REG_ID_PUE: // gpio input pull enable
	case REG_ID_PUD: // gpio input pull direction
	{
        #ifdef BEEPY
		if (is_write) {
			switch (reg) {
			case REG_ID_DIR:
				gpioexp_update_dir(in_data);
				break;
			case REG_ID_PUE:
				gpioexp_update_pue_pud(in_data, reg_get_value(REG_ID_PUD));
				break;
			case REG_ID_PUD:
				gpioexp_update_pue_pud(reg_get_value(REG_ID_PUE), in_data);
				break;
			}
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}
        #endif
		break;
	}

    // charger interface (charging power, charging enable)
    #ifdef BLEPIS
    case REG_ID_PWR: // power settings/state
    {
		if (is_write) {
            // only react if the bits have changed
            bool chgdis_set = reg_is_bit_set(REG_ID_PWR, PWR_CHGDIS);
            bool chgpwr_set = reg_is_bit_set(REG_ID_PWR, PWR_CHGPWR);
			reg_set_value(reg, in_data);
            if (reg_is_bit_set(REG_ID_PWR, PWR_CHGDIS) != chgdis_set) {
                if (reg_is_bit_set(REG_ID_PWR, PWR_CHGDIS)) {
                    charger_disable();
                } else {
                    charger_enable();
                }
            }
            if (reg_is_bit_set(REG_ID_PWR, PWR_CHGPWR) != chgpwr_set) {
                if (reg_is_bit_set(REG_ID_PWR, PWR_CHGPWR)) {
                    charger_hipwr();
                } else {
                    charger_lopwr();
                }
            }
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
        }
		break;
    }

    case REG_ID_MUX: // power settings/state
    {
		if (is_write) {
            // only react if the bits have changed
            bool muxusb_set = reg_is_bit_set(REG_ID_MUX, MUX_USB);
            bool muxfusb_set = reg_is_bit_set(REG_ID_MUX, MUX_FUSB);
			reg_set_value(reg, in_data);
            if (reg_is_bit_set(REG_ID_MUX, MUX_USB) != muxusb_set) {
                if (reg_is_bit_set(REG_ID_MUX, MUX_USB)) {
                    usbmux_host();
                } else {
                    usbmux_rp2040();
                }
            }
            if (reg_is_bit_set(REG_ID_MUX, MUX_FUSB) != muxfusb_set) {
                if (reg_is_bit_set(REG_ID_MUX, MUX_FUSB)) {
                    fusbmux_zero();
                } else {
                    fusbmux_rp2040();
                }
            }
            #ifdef BLEPIS_V2
            bool muxuart_set = reg_is_bit_set(REG_ID_MUX, MUX_UART);
            if (reg_is_bit_set(REG_ID_MUX, MUX_UART) != muxuart_set) {
                if (reg_is_bit_set(REG_ID_MUX, MUX_UART)) {
                    uartmux_intl();
                } else {
                    uartmux_exp();
                }
            }
            #endif
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
        }
		break;
    }
    #endif

	case REG_ID_TOUCHPAD_REG:
		if (is_write) {
			reg_set_value(reg, in_data);
		}
		break;

	case REG_ID_TOUCHPAD_VAL:
		if (is_write) {
			touchpad_write_i2c_u8(reg_get_value(REG_ID_TOUCHPAD_REG), in_data);
		} else {
			out_buffer[0] = touchpad_read_i2c_u8(reg_get_value(REG_ID_TOUCHPAD_REG));
			*out_len = sizeof(uint8_t);
		}
		break;

	case REG_ID_TOUCHPAD_LED:
		if (is_write) {
			touchpad_set_led_power(in_data);
			reg_set_value(reg, in_data);
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}

	case REG_ID_GIO: // gpio value
	{
        #ifdef BEEPY
		if (is_write) {
			gpioexp_set_value(in_data);
		} else {
			out_buffer[0] = gpioexp_get_value();
			*out_len = sizeof(uint8_t);
		}
		break;
        #endif
	}

	// RGB LED registers
	case REG_ID_LED_R:
	case REG_ID_LED_G:
	case REG_ID_LED_B:
	{
		if (is_write) {
			reg_set_value(reg, in_data);
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	case REG_ID_LED:
	{
		if (is_write) {
			reg_set_value(reg, in_data);
			struct led_state state;
			state.setting = (enum led_setting)in_data;
			state.r = reg_get_value(REG_ID_LED_R);
			state.g = reg_get_value(REG_ID_LED_G);
			state.b = reg_get_value(REG_ID_LED_B);
			led_set(&state);
		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	// Rewake on timer
	case REG_ID_REWAKE_MINS:
	{
		// Value of zero will cancel alarms
		if (in_data == 0) {
			pi_cancel_power_alarms();

			// Reset startup reason if in rewake
			if (reg_get_value(REG_ID_STARTUP_REASON) == POWER_ON_REWAKE) {
				reg_set_value(REG_ID_STARTUP_REASON, POWER_ON_REWAKE_CANCELED);
			}

			break;
		}

		// Only run this if driver was loaded
		// Otherwise, OS won't get the power key event
		if (reg_get_value(REG_ID_DRIVER_STATE) == 0) {
			break;
		}

		// Get rewake and grace times in milliseconds
		uint32_t rewake_ms = in_data * 60 * 1000;
		uint32_t shutdown_grace_ms = reg_get_shutdown_grace_ms();

		// Check input time against shutdown grace time
		// Plus some slop to allow for power cycling
		if (rewake_ms < (shutdown_grace_ms + 5000)) {
			break;
		}

		// Send shutdown signal to OS
		keyboard_inject_power_key();

		// Power off with grace time to give Pi time to shut down
		pi_schedule_power_off(0, shutdown_grace_ms, false /* live */);

		// Schedule power on
		pi_schedule_power_on(rewake_ms);

		break;
	}

	// Real time clock
	case REG_ID_RTC_SEC:
	case REG_ID_RTC_MIN:
	case REG_ID_RTC_HOUR:
	case REG_ID_RTC_MDAY:
	case REG_ID_RTC_MON:
	case REG_ID_RTC_YEAR:
	{
		if (is_write) {
			reg_set_value(reg, in_data);
		} else {
			out_buffer[0] = rtc_get(reg);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	case REG_ID_RTC_COMMIT:
	{
		rtc_set(reg_get_value(REG_ID_RTC_YEAR), reg_get_value(REG_ID_RTC_MON),
			reg_get_value(REG_ID_RTC_MDAY), reg_get_value(REG_ID_RTC_HOUR),
			reg_get_value(REG_ID_RTC_MIN), reg_get_value(REG_ID_RTC_SEC));
		break;
	}

	case REG_ID_UPDATE_DATA:
	{
		if (is_write) {

			if ((rc = update_recv(in_data))) {

				// More to read or update failed
				reg_set_value(REG_ID_UPDATE_DATA, (rc < 0)
					? (uint8_t)(-rc)
					: UPDATE_RECV);

			// Update read successfully
			} else {

				reg_set_value(REG_ID_UPDATE_DATA, UPDATE_OFF);

				// Send shutdown signal to OS
				keyboard_inject_power_key();

				// Power off with grace time to give Pi time to shut down
				uint32_t shutdown_grace_ms = reg_get_shutdown_grace_ms();
				pi_schedule_power_off(0, shutdown_grace_ms, false /* live */);
				add_alarm_in_ms(shutdown_grace_ms + 10,
					update_commit_alarm_callback, NULL, true);
			}

		} else {
			out_buffer[0] = reg_get_value(REG_ID_UPDATE_DATA);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	case REG_ID_DRIVER_STATE:
	{
		if (is_write) {
			reg_set_value(reg, in_data);

			// Driver unloaded, if auto off schedule a shutdown, power off, and sleep
			if ((in_data == 0) && (reg_get_value(REG_ID_CF2) && CF2_AUTO_OFF)) {
				pi_schedule_power_off(30*1000, reg_get_shutdown_grace_ms(),
					true /* dormant */);

			// Driver loaded, cancel shutdown and power off
			} else if (in_data) {
				pi_cancel_power_alarms();

				// Clear any input queued while driver was unloaded
				fifo_flush();
			}

		} else {
			out_buffer[0] = reg_get_value(reg);
			*out_len = sizeof(uint8_t);
		}
		break;
	}

	// read-only registers
	case REG_ID_TOX:
	case REG_ID_TOY:
		out_buffer[0] = reg_get_value(reg);
		*out_len = sizeof(uint8_t);

		reg_set_value(reg, 0);
		break;

	case REG_ID_VER:
		out_buffer[0] = VER_VAL;
		*out_len = sizeof(uint8_t);
		break;

	case REG_ID_ADC:
		adc_value = adc_read();
		out_buffer[0] = (uint8_t)(adc_value & 0x00FF);
		out_buffer[1] = (uint8_t)((adc_value & 0xFF00) >> 8);
		*out_len = sizeof(uint8_t) * 2;
		break;

	case REG_ID_KEY:
		out_buffer[0] = fifo_count();
		*out_len = sizeof(uint8_t);
		break;

	case REG_ID_FIF:
	{
		struct fifo_item item = fifo_dequeue();

		out_buffer[0] = ((uint8_t*)&item)[0];
		out_buffer[1] = ((uint8_t*)&item)[1];
		*out_len = sizeof(uint8_t) * 2;
		break;
	}

	case REG_ID_RST:
		NVIC_SystemReset();
		break;

	case REG_ID_STARTUP_REASON:
		out_buffer[0] = reg_get_value(reg);
		*out_len = sizeof(uint8_t);
		break;
	}
}

uint8_t reg_get_value(enum reg_id reg)
{
#ifdef DEBUG_REGS
	printf("%s: reg: 0x%02X, val: 0x%02X\r\n", __func__, reg, self.regs[reg]);
#endif
	return self.regs[reg];
}

uint8_t reg_get_value_quiet(enum reg_id reg)
{
	return self.regs[reg];
}

void reg_set_value(enum reg_id reg, uint8_t value)
{
#ifdef DEBUG_REGS
	printf("%s: reg: 0x%02X, val: 0x%02X (%d)\r\n", __func__, reg, value, value);
#endif

	self.regs[reg] = value;
}

bool reg_is_bit_set(enum reg_id reg, uint8_t bit)
{
	return self.regs[reg] & bit;
}

void reg_set_bit(enum reg_id reg, uint8_t bit)
{
#ifdef DEBUG_REGS
	printf("%s: reg: 0x%02X, bit: %d\r\n", __func__, reg, bit);
#endif

	self.regs[reg] |= bit;
}

void reg_clear_bit(enum reg_id reg, uint8_t bit)
{
#ifdef DEBUG_REGS
	printf("%s: reg: 0x%02X, bit: %d\r\n", __func__, reg, bit);
#endif

	self.regs[reg] &= ~bit;
}

void reg_init(void)
{
	reg_set_value(REG_ID_CFG, CFG_OVERFLOW_INT | CFG_KEY_INT | CFG_USE_MODS);
	reg_set_value(REG_ID_BKL, 0x16);
	reg_set_value(REG_ID_DEB, 10);
	reg_set_value(REG_ID_FRQ, 10);	// ms
	reg_set_value(REG_ID_BK2, 255);
	reg_set_value(REG_ID_PUD, 0xFF);
	reg_set_value(REG_ID_HLD, 100);	// 10ms units
	reg_set_value(REG_ID_ADR, 0x1F);
	reg_set_value(REG_ID_IND, 1);	// ms
    reg_set_value(REG_ID_CF2, CF2_TOUCH_INT | CF2_USB_KEYB_ON | CF2_USB_MOUSE_ON);
	//reg_set_value(REG_ID_CF2, 0);
	reg_set_value(REG_ID_DRIVER_STATE, 0); // Driver not yet loaded

	reg_set_value(REG_ID_SHUTDOWN_GRACE, 30);

	reg_set_value(REG_ID_TOUCHPAD_MIN_SQUAL, 16);

	reg_set_value(REG_ID_MUX, 0x02); // fusb mux high by default, usb mux low

	touchpad_add_touch_callback(&touch_callback);
}


uint32_t reg_get_shutdown_grace_ms()
{
	return MAX(reg_get_value(REG_ID_SHUTDOWN_GRACE) * 1000,
		MINIMUM_SHUTDOWN_GRACE_MS);
}

