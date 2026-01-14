#include "pi.h"
#include "gpio.h"
#include "usb.h"
#include "reg.h"
#include "keyboard.h"
#include "backlight.h"
#include "fifo.h"
#include "hardware/adc.h"
#include <hardware/pwm.h>

#ifdef BEEPY
#include "gpioexp.h"
#endif

#ifdef BLEPIS
#include "peripherals.h"
#include "ws2812.pio.h"
#include "hardware/pio.h"
#endif

#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/rtc.h"
#include "hardware/structs/scb.h"

#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <pico/runtime_init.h>
#ifndef NDEBUG
    #include <stdio.h>
#endif

#define LED_FLASH_CYCLE_MS 3000
#define LED_FLASH_ON_MS 200

// Globals
alarm_id_t g_power_on_alarm = -1;
alarm_id_t g_shutdown_alarm = -1;
alarm_id_t g_power_off_alarm = -1;
uint8_t g_dormant_reentry = 0;

struct led_state g_led_state;
struct led_state g_led_flash_state;
alarm_id_t g_led_flash_alarm = -1;

enum pi_state
{
	PI_STATE_OFF = 0,
	PI_STATE_ON = 1,
};

static enum pi_state g_pi_state;

void pi_power_init(void)
{
	adc_init();
	adc_gpio_init(PIN_BAT_ADC);
	adc_select_input(0);

	uni_gpio_init(PIN_PI_PWR);
	uni_gpio_set_dir(PIN_PI_PWR, GPIO_OUT);
	uni_gpio_put(PIN_PI_PWR, 0);
    #ifdef BLEPIS
	    uni_gpio_set_dir(PIN_DISP_RST, GPIO_OUT);
    	uni_gpio_put(PIN_DISP_RST, 1);
    #endif
	g_pi_state = PI_STATE_OFF;
}

void pi_power_on(enum power_on_reason reason)
{
	struct led_state state;

	if (g_pi_state == PI_STATE_ON) {
		return;
	}

    #ifdef BLEPIS
    	uni_gpio_put(PIN_DISP_RST, 1); //display RST starts out deasserted
    #endif
	uni_gpio_put(PIN_PI_PWR, 1);
	g_pi_state = PI_STATE_ON;

	// Clear any input queued while Pi was off
	fifo_flush();

	// LED green while booting until driver loaded
	state.setting = LED_SET_ON;
	state.r = 0;
	state.g = 128;
	state.b = 0;
	led_set(&state);

	// Update startup reason
	reg_set_value(REG_ID_STARTUP_REASON, reason);
    // Blepis-specific stuff - 5V boost control if appropriate, and display reset toggle
    #ifdef BLEPIS
        #ifdef BLEPIS_V2
            boost_enable(); // enabling boost a little bit before powering up the Pi
        #endif
    	uni_gpio_put(PIN_DISP_RST, 0); // Assert display RESET
    	// Wait a little and bring the display out of RESET
	    sleep_ms(500);
    	uni_gpio_put(PIN_DISP_RST, 1);
    #endif
}

void pi_power_off(void)
{
	if (g_pi_state == PI_STATE_OFF) {
		return;
	}

	uni_gpio_put(PIN_PI_PWR, 0);
	g_pi_state = PI_STATE_OFF;
}

static int64_t pi_power_on_alarm_callback(alarm_id_t _, void* enum_reason)
{
	if (g_power_on_alarm < 0) {
		return 0;
	}

	pi_cancel_power_alarms();
	pi_power_on((enum power_on_reason)enum_reason);

	return 0;
}

void pi_reboot(enum power_on_reason reason)
{
	// Turn off Pi
	pi_power_off();

	// Cancel existing alarm if scheduled
	if (g_power_on_alarm >= 0) {
		cancel_alarm(g_power_on_alarm);
		g_power_on_alarm = -1;
	}

	// Schedule new alarm aftel allowing time for Pi to power off
	g_power_on_alarm = add_alarm_in_ms(500, pi_power_on_alarm_callback, (void*)reason, true);
}

void pi_schedule_power_on(uint32_t ms)
{
	// Cancel existing alarm if scheduled
	if (g_power_on_alarm >= 0) {
		cancel_alarm(g_power_on_alarm);
		g_power_on_alarm = -1;
	}

	// Schedule new alarm
	g_power_on_alarm = add_alarm_in_ms(ms, pi_power_on_alarm_callback,
		(void*)POWER_ON_REWAKE, true);
}

static int64_t pi_shutdown_alarm_callback(alarm_id_t _, void* __)
{
	if (g_shutdown_alarm < 0) {
		return 0;
	}

	keyboard_inject_power_key();

	return 0;
}

static int64_t pi_power_off_alarm_callback(alarm_id_t _, void* int_dormant)
{
	uint8_t dormant = (int_dormant) ? 1 : 0;

	if (g_power_off_alarm < 0) {
		return 0;
	}

	pi_power_off();
	if (dormant) {
		g_dormant_reentry = dormant;
		dormant_until_power_key_down();
	}

	return 0;
}

void pi_schedule_power_off(uint32_t shutdown_ms, uint32_t poweroff_ms, uint8_t dormant)
{
	// Cancel shutdown alarm
	if (g_shutdown_alarm >= 0) {
		cancel_alarm(g_shutdown_alarm);
		g_shutdown_alarm = -1;
	}

	// Cancel power off alarm
	if (g_power_off_alarm >= 0) {
		cancel_alarm(g_power_off_alarm);
		g_power_off_alarm = -1;
	}

	// Schedule shutdown alarm
	if (shutdown_ms < 10) {
		shutdown_ms = 10;
	}
	g_shutdown_alarm = add_alarm_in_ms(shutdown_ms,
		pi_shutdown_alarm_callback, NULL, true);

	// Schedule poweroff alarm
	g_power_off_alarm = add_alarm_in_ms(shutdown_ms + poweroff_ms,
		pi_power_off_alarm_callback, (void*)((dormant) ? 1 : 0), true);
}

void pi_cancel_power_alarms()
{
	// Cancel shutdown alarm
	if (g_shutdown_alarm >= 0) {
		cancel_alarm(g_shutdown_alarm);
		g_shutdown_alarm = -1;
	}

	// Cancel power off alarm
	if (g_power_off_alarm >= 0) {
		cancel_alarm(g_power_off_alarm);
		g_power_off_alarm = -1;
	}

	// Cancel power on alarm
	if (g_power_on_alarm >= 0) {
		cancel_alarm(g_power_on_alarm);
		g_power_on_alarm = -1;
	}
}

//static inline
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) |
         ((uint32_t)(g) << 16) |
         (uint32_t)(b);
}

//static inline
/*
uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return  ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            ((uint32_t) (w) << 24) |
            (uint32_t) (b);
}
*/

uint32_t pixel_grb = 0;
uint32_t dbg_pixel_grb = 0;

#ifdef PIN_NEO_PIXEL
static inline void put_pixel() {
    #ifdef BLEPIS_V2
      // two neopixels, first one is debug pixel and needs to keep getting updated
      pio_sm_put_blocking(pio0, 0, dbg_pixel_grb << 8u);
    #endif
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
  }

#endif

//static inline 
void dbg_light(uint32_t dbg_light_grb) {
  #ifdef BLEPIS_V2
    dbg_pixel_grb = dbg_light_grb;
    put_pixel();
  #endif
}

static void led_sync(bool enable, uint8_t r, uint8_t g, uint8_t b)
{
	#ifdef PIN_NEO_PIXEL
	if (!enable) {
        #ifndef NDEBUG
            printf("led: 0x%02X/0x%02X/0x%02X\r\n", 0, 0, 0);
        #endif
        pixel_grb = urgb_u32(0x00,0x00,0x00); //,0x00);
		put_pixel();
		return;
	}
    #ifndef NDEBUG
        printf("led: 0x%02X/0x%02X/0x%02X\r\n", r, g, b);
    #endif

    pixel_grb = urgb_u32(r,g,b); //,255);
	put_pixel();
	#endif
	#ifndef PIN_NEO_PIXEL
	if (!enable) {
		pwm_set_gpio_level(PIN_LED_R, 0xFFFF);
		pwm_set_gpio_level(PIN_LED_G, 0xFFFF);
		pwm_set_gpio_level(PIN_LED_B, 0xFFFF);
		return;
	}
	// Set the PWM slice for each channel
	uint slice_r = pwm_gpio_to_slice_num(PIN_LED_R);
	uint slice_g = pwm_gpio_to_slice_num(PIN_LED_G);
	uint slice_b = pwm_gpio_to_slice_num(PIN_LED_B);

	// Calculate the PWM value for each channel
	uint16_t pwm_r = (0xFF - r) * 0x101;
	uint16_t pwm_g = (0xFF - g) * 0x101;
	uint16_t pwm_b = (0xFF - b) * 0x101;

	// Set the PWM duty cycle for each channel
	pwm_set_gpio_level(PIN_LED_R, pwm_r);
	pwm_set_gpio_level(PIN_LED_G, pwm_g);
	pwm_set_gpio_level(PIN_LED_B, pwm_b);

	// Enable PWM channels
	pwm_set_enabled(slice_r, true);
	pwm_set_enabled(slice_g, true);
	pwm_set_enabled(slice_b, true);
	#endif

}

void led_init(void)
{
	#ifndef PIN_NEO_PIXEL
	// Set up PWM channels
	gpio_set_function(PIN_LED_R, GPIO_FUNC_PWM);
	gpio_set_function(PIN_LED_G, GPIO_FUNC_PWM);
	gpio_set_function(PIN_LED_B, GPIO_FUNC_PWM);
	#endif
    #ifdef PIN_NEO_PIXEL
	PIO pio = pio0;
  	int sm = 0;
 	uint offset = pio_add_program(pio, &ws2812_program);

	ws2812_program_init(pio, sm, offset, PIN_NEO_PIXEL, 800000, false);
	#endif

}

void led_test(void) {
  #ifdef PIN_NEO_PIXEL
    pixel_grb = urgb_u32(255,255,255);
    put_pixel();
  #endif
}

static int64_t pi_led_flash_alarm_callback(alarm_id_t _, void* __)
{
	static bool led_enabled = false;
	uint32_t alarm_ms;

	// Flash canceled
	if (g_led_flash_alarm < 0) {
		return 0;
	}

	// Toggle LED
	led_enabled = !led_enabled;
	if (g_led_flash_state.setting == LED_SET_FLASH_UNTIL_KEY) {

		// Switch between flash until key and base LED setting
		if (led_enabled) {
			led_sync(true, g_led_flash_state.r, g_led_flash_state.g,  g_led_flash_state.b);
		} else {
			led_sync((g_led_state.setting == LED_SET_ON), g_led_state.r, g_led_state.g,  g_led_state.b);
		}

	// Regular flash
	} else if (g_led_state.setting == LED_SET_FLASH_ON) {
		led_sync(led_enabled, g_led_state.r, g_led_state.g,  g_led_state.b);

	// Flash canceled
	} else {
		led_sync((g_led_state.setting == LED_SET_ON), g_led_state.r, g_led_state.g,  g_led_state.b);
		g_led_flash_alarm = -1;
		return 0;
	}

	// Reschedule timer
	alarm_ms = (led_enabled)
		? LED_FLASH_ON_MS
		: (LED_FLASH_CYCLE_MS - LED_FLASH_ON_MS);
	g_led_flash_alarm = add_alarm_in_ms(alarm_ms, pi_led_flash_alarm_callback, NULL, true);

	return 0;
}

static void pi_led_stop_flash_alarm_callback(uint8_t key, enum key_state state)
{
	// Don't restore if power key (sent during shutdown)
	if (key == KEY_POWER) {
		return;
	}

	// Restore original LED state
	g_led_flash_state.setting = LED_SET_OFF;

	// Remove key callback
	keyboard_remove_key_callback(pi_led_stop_flash_alarm_callback);
}
static struct key_callback pi_led_stop_flash_key_callback = {
	.func = pi_led_stop_flash_alarm_callback,
	.next = NULL
};

void led_set(struct led_state const* state)
{
	// Store LED state
	if (state->setting == LED_SET_FLASH_UNTIL_KEY) {
		g_led_flash_state = *state;
	} else {
		g_led_state = *state;
	}

	// Schedule flash callback
	if ((state->setting == LED_SET_FLASH_ON) || (state->setting == LED_SET_FLASH_UNTIL_KEY)) {

		// Apply LED setting and schedule new timer using callback
		if (g_led_flash_alarm < 0) {
			g_led_flash_alarm = 1;
			(void)pi_led_flash_alarm_callback(0, NULL);
		}

		// Add key calback to disable flash when key is pressed
		if (state->setting == LED_SET_FLASH_UNTIL_KEY) {
			keyboard_add_key_callback(&pi_led_stop_flash_key_callback);
		}

	// Regular on / off LED setting
	} else {
		led_sync((state->setting == LED_SET_ON), state->r, state->g,  state->b);
	}
}

struct sleep_state
{
	uint8_t keyboard_backlight;
	struct led_state led_state;
	uint scb_orig, clock0_orig, clock1_orig;
};

static void sleep_prepare(struct sleep_state* ss)
{
	// Save backlight and LED state
	ss->keyboard_backlight = reg_get_value(REG_ID_BKL);
	ss->led_state = g_led_state;

	// Save existing clock states
	ss->scb_orig = scb_hw->scr;
	ss->clock0_orig = clocks_hw->sleep_en0;
	ss->clock1_orig = clocks_hw->sleep_en1;

	// Clear LED and backlight
	reg_set_value(REG_ID_BKL, 0);
	led_sync(true, 0, 0, 0);
}

static void sleep_resume(struct sleep_state const* ss)
{
	// Recover from sleep
	rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
	scb_hw->scr = ss->scb_orig;
	clocks_hw->sleep_en0 = ss->clock0_orig;
	clocks_hw->sleep_en1 = ss->clock1_orig;
    runtime_init_clocks();

	// Restore LED and keyboard states
	led_set(&ss->led_state);
	reg_set_value(REG_ID_BKL, ss->keyboard_backlight);

    // Restart USB callback - experimental
    //reinit_usb();
}

void dormant_until_power_key_down(void)
{
	datetime_t t;
	struct sleep_state ss;

	// Invalidate RTC
	t.year = 1970;
	t.month = 1;
	t.day = 1;
	t.dotw = 4;
	t.hour = 0;
	t.min = 0;
	t.sec = 0;

	// Save clocks, LED, backlight
	sleep_prepare(&ss);

	// Sleep until power key is pressed
	sleep_run_from_xosc();
	sleep_goto_dormant_until_pin(POWER_BTN, 0, 0);

	// Restore clocks, LED, backlight
	sleep_resume(&ss);
}

void dormant_set_reentry_flag(uint8_t value)
{
	g_dormant_reentry = value;
}

uint8_t dormant_get_reentry_flag(void)
{
	return g_dormant_reentry;
}

static void sleep_callback(void)
{}

void dormant_seconds(int seconds)
{
	struct sleep_state ss;
	datetime_t t;

	// Save clocks, LED, backlight
	sleep_prepare(&ss);

	// Get datetime offset
	rtc_get_datetime(&t);
	t.sec += seconds;
	while (t.sec >= 60) {
		t.sec -= 60;
		t.min += 1;
	}
	while (t.min >= 60) {
		t.min -= 60;
		t.hour += 1;
	}
	// May not work correctly around midnight...
	while (t.hour >= 24) {
		t.hour -= 24;
		t.day += 1;
		t.dotw = (t.dotw + 1) & 7;
	}

	sleep_run_from_xosc();
	sleep_goto_sleep_until(&t, sleep_callback);

	// Advance RTC
	rtc_set_datetime(&t);

	// Restore clocks, LED, backlight
	sleep_resume(&ss);
}
