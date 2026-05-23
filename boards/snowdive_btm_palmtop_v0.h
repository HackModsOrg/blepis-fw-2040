// The pins of the XL are numbered as if they are part of the RP so A0 is 30 , B0 is 38 and B7 is 46
#pragma once

#define SNOWDIVE
#define SNOWDIVE_BTM_PALMTOP
#define SNOWDIVE_BTM_PALMTOP_V0

#define USB_VID				0x1209
#define USB_PID				0xB182
#define USB_PRODUCT			"Snowdive BTM V0"

// CPU-connected bus
#define PIN_PUPPET_SDA		28 //
#define PIN_PUPPET_SCL		29 //
#define PIN_INT				27 //

// internal touchpad and other I2C device bus
#define PIN_SDA				18 //
#define PIN_SCL				23 //
#define PIN_TP_RESET		44 //
#define PIN_TP_MOTION		22 //
#define PIN_TP_SHUTDOWN		45 //

// IO expanders

#define XL9535_BTM_LEFT_ADDR 0x26 // U21, addr range from 30 to 45 (inclusive)
#define XL9535_BTM_RIGHT_ADDR 0x27 // U28, addr range from 46 to 61 (inclusive)
#define XL9535_TOP_LEFT_ADDR 0x24 // U27, addr range from 62 to 77 (inclusive)
#define XL9535_TOP_RIGHT_ADDR 0x25 // U25, addr range from 78 to 93 (inclusive)

#define PIN_XL9535_BOTTOM_INT  17 // two expander INTs joined together, bottom board
#define PIN_XL9535_TOP_INT  16 // two expander INTs joined together, bottom board

// very important pin, switches the two top board IO expanders to EC control instead of CPU control
#define PIN_IO_MUX_SEL 38 //

/*

POWER and USB-C

*/

#define PIN_3V3_EC_EN 52 // very important, needed for the top IO expanders to show up
#define PIN_PI_PWR 62 // # TOP BOARD
#define PIN_ESP_PWR 65 // # TOP BOARD
#define PIN_5V_BOOST_EN 72 // 0x52 // # TOP BOARD
#define PIN_5V_PGOOD 73 // 0x52 // # TOP BOARD
#define PIN_5V_BTM_PGOOD 54 // 0x52 //
#define PIN_VBUS_DET 42 //

#define PIN_CHG_DIS 55 // 0x52 //
#define PIN_CHG_PWR 56 // 0x52 //
#define PIN_VBUS_BYPASS 59 //
#define PIN_VINB_PGOOD 58 //
#define PIN_VINB_EN 57 // inverted!

#define PIN_BAT_ADC 26 //

// USB-C

#define FUSB302_ADDR 0x23 // new FUSB is the B01 variant, to avoid conflicts with the top FUSB302
#define FUSB302_TOP_ADDR 0x22 // top board, may not be present depending on the FUSB mux state
#define PIN_FUSB_INT 43 //
#define PIN_FUSB_MUX_SEL 75 // 0x51 // TOP BOARD
#define PIN_FUSB_TOP_INT 77 // TOP BOARD

// two USB-C ports on the sides
#define WUSB_LEFT_ADDR 0x60
#define WUSB_LEFT_INT 30 //
#define WUSB_LEFT_PWR 31 //
#define WUSB_RIGHT_ADDR 0x68
#define WUSB_RIGHT_INT 60 //
#define WUSB_RIGHT_PWR 61 //

// 1 TMP102 sensor onboard, U34 0x4b. will be used for monitoring power circuitry local temperature
#define TEMP_BASE_ADDR 0x4b

/*

PERIPHERALS

*/

// Snowdive uses Neopixels just like Blepis
#define PIN_NEO_PIXEL 20 //
#define NEOPIXEL_AMOUNT 4 // two on bottom board, two on top board

// ESP32-C6 pins
#define PIN_ESP_EN 63 // # TOP BOARD
#define PIN_ESP_BOOT 64 // # TOP BOARD

#define PIN_BUZZER 19 //
#define PIN_VIBRO_DRV 21 // 0x50 //
#define PIN_DISP_EXTIN 68 // 0x53 // # TOP BOARD
#define PIN_DISP_RST  69 // 0x53 // # TOP BOARD
// backlights (currently driven exclusively together)
#define PIN_BKL	      25 // BKL //
// LCD-specific backlight
#define PIN_LCDBL_DRV 24 // BKL //

#define PIN_UART_MUX_SEL 66 // TOP BOARD
#define PIN_I2C_PU_PWR 37 //
#define PIN_RTC_INT 36 //

// defining a testpointed (non-pulled) pin so it can be set to output to avoid wasting time on possible interrupts
#define PIN_UNUSED1 35 //

// on Snowdive, there's two main USB muxes: a pair on the bottom board and a pair on the top board.
// the top board muxes switch between device and host mode (which is kinda what the driver expects)
// specifically, the two muxes switch a USB hub in/out.
// the bottom board muxes connect the RP2040 to one of the bottom-cable-exposed USB-C ports by default,
// which is useful for failsafe mode (keyboard and mouse support before the beepy-kbd driver has loaded).

#define PIN_USB_MUX_BTM_S_EN 39 // "USBC_S_MUX_EN"
#define PIN_USB_MUX_BTM_S_SEL 40 // "USBC_S_MUX", these two have to be driven in unison and inverted. maybe v1 fixes that
#define PIN_USB_MUX_BTM_C_SEL 41 // "USBC_C_MUX"
#define PIN_USB_MUX_TOP_C_SEL 76 // TOP BOARD, "USBC_C_MUX". these two have to be driven in unison and inverted. maybe v1 fixes that
#define PIN_USB_MUX_TOP_S_SEL 74 // TOP BOARD, "USBC_S_MUX".
//#define PIN_USB_MUX_EN ?? // TOP BOARD, but not present. maybe oversight? yet unused but I've had plans.

#define PIN_TAL 53 //
#define PIN_USBC_IN_EN 90 // TOP BOARD, "USBC_SNK_EN"
#define PIN_USBC_OUT_EN 88 // TOP BOARD, "USBC_SRC_EN"
#define PIN_USBC_IN_FLG 91 // TOP BOARD, "USBC_SNK_FLG"
#define PIN_USBC_OUT_FLG 89 // TOP BOARD, "USBC_SRC_FLG"

// on Snowdive, there's also an LTE modem

#define PIN_MODEM_PWR_EN 93 // 0x52 // # TOP BOARD
#define PIN_MODEM_PWR_PGOOD 92 // 0x52 // # TOP BOARD
#define PIN_MODEM_IO_PWRKEY 80 // 0x52 // # TOP BOARD
#define PIN_MODEM_IO_RST 81 // 0x52 // # TOP BOARD
#define PIN_MODEM_IO_BOOTCFG 82 // 0x52 // # TOP BOARD

// and there's an optional TPM we can reset!

#define PIN_TPM_RST 79 // 0x52 // # TOP BOARD

//PIN_PROG_DET not present
//PIN_CHG_DET not present
//PIN_CLED_EN not present
//PIN_USBC_OUT_PWR_EN not present

/*

KEYBOARD and BUTTONS

*/

// keyboard matrix

// side buttons
#define PIN_BUTTON_3 32 //
#define PIN_BUTTON_2 33 //
#define PIN_BUTTON_1 34 //
// top buttons
#define PIN_TBUTTON_1 51 //
#define PIN_TBUTTON_2 50 //
#define PIN_TBUTTON_3 49 //
#define PIN_TBUTTON_4 48 //
#define PIN_TBUTTON_5 47 //
#define PIN_TBUTTON_6 46 //

#define NUM_OF_ROWS			8
#define PINS_COLS \
	8,  \
	10, \
	12, \
	14, \
	9,  \
	11, \
	13, \
    15

#define NUM_OF_COLS			8
#define PINS_ROWS \
	6, \
	5, \
	4, \
	3, \
	2, \
	1, \
    0, \
    7

#define NUM_OF_BTNS			0 // buttons not yet added in because there's no support for IO expander read yet. for now.
#define POWER_BTN           3
#define PINS_BTNS \
	POWER_BTN, // COL7
#define BTN_KEYS \
	{ KEY_POWER },

#define PIN_GPIOEXP0		PIN_PI_SHUTDOWN

#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
