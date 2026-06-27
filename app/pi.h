#pragma once

#include <stdint.h>
#include <stdbool.h>

enum power_on_reason
{
	POWER_ON_FW_INIT = 0x1, // RP2040 initialized
	POWER_ON_BUTTON = 0x2, // Power button held after Pi was shut down
	POWER_ON_REWAKE = 0x3, // Pi was reawakened from a scheduled timer
	POWER_ON_REWAKE_CANCELED = 0x4 // Rewake timer was canceled
};

#define MINIMUM_SHUTDOWN_GRACE_MS 5000

void pi_power_init(bool already_powered);
void pi_power_on(enum power_on_reason reason);
void pi_power_off(void);
void pi_reboot(enum power_on_reason reason);

void pi_schedule_power_on(uint32_t ms);
void pi_schedule_power_off(uint32_t shutdown_ms, uint32_t poweroff_ms, uint8_t dormant);
void pi_cancel_power_alarms();

enum led_setting
{
	LED_SET_OFF = 0x0,
	LED_SET_ON = 0x1,
	LED_SET_FLASH_ON = 0x2,
	LED_SET_FLASH_UNTIL_KEY = 0x3
};

struct led_state
{
	enum led_setting setting;
	uint8_t r, g, b;
};

void dbg_light(uint32_t dbg_light_grb);
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

void led_init(void);
void led_test(void);
void led_set(struct led_state const* state);

void dormant_until_power_key_down(void);
void dormant_set_reentry_flag(uint8_t value);
uint8_t dormant_get_reentry_flag(void);

bool check_pi_powered(void);
