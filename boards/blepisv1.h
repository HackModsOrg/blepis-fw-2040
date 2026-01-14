// We are probly not gonna be compat with beepy or the bbq20kdb breadkout but maybe ...
// The PINs of the MCP are numbered as if they are part of the rp so A0 is 30 , B0 is 38 and B7 is 46
#pragma once

#define USB_VID				0x1209
#define USB_PID				0xB182
#define USB_PRODUCT			"BBQ20KBD"

#define PIN_INT				0
#define PIN_BKL				25

#define PIN_SDA				18
#define PIN_SCL				23

#define PIN_TP_RESET		37
#define PIN_TP_MOTION		22
#define PIN_TP_SHUTDOWN		33

#define PIN_PUPPET_SDA		28
#define PIN_PUPPET_SCL		29

#define BLEPIS
#define BLEPIS_V1

/** beeper specific pins **/
#define PIN_PI_PWR 35
#define PIN_PI_SHUTDOWN 45
/** Bepis use NEOPIXEL
#define PIN_LED_R 20
#define PIN_LED_G 19
#define PIN_LED_B 17
**/
#define PIN_BAT_ADC 26

/** Bepis specific pins and config **/

#define PIN_USB_MUX_SEL 30 // 0x51 //
#define PIN_FUSB_MUX_SEL 31 // 0x51 //
#define PIN_BUZZER 15
#define PIN_MCP_REST 17 // internal
#define PIN_MCP_INT  16 // EX_INT in schematic
#define PIN_NEO_PIXEL 20 // internal
#define PIN_VIBRO_DRV 21 // 0x50 //
#define PIN_LCDBL_DRV 24 // BKL //
#define PIN_DISP_EXTIN 38 // 0x53 //
#define PIN_DISP_RST 39  // 0x53 //
#define PIN_CHG_DIS 32 // 0x52 //
#define PIN_CHG_PWR 34 // 0x52 //
#define PIN_5V_PGOOD 36 // 0x52 //

#define MCP_ADDR 0x22

#define NUM_OF_ROWS			7
#define PINS_ROWS \
	14, \
	13, \
	12, \
	11, \
	10, \
	9, \
	8

#define NUM_OF_COLS			6
#define PINS_COLS \
	1,  \
	2,  \
	3, \
	7, \
	6, \
	5

#define NUM_OF_BTNS			1
#define POWER_BTN           4
#define PINS_BTNS \
	POWER_BTN,
#define BTN_KEYS \
	{ KEY_POWER },

#define PIN_GPIOEXP0		PIN_PI_SHUTDOWN
// #define PIN_GPIOEXP1		17
// #define PIN_GPIOEXP2		19
// #define PIN_GPIOEXP3		21
// #define PIN_GPIOEXP4		26

// #define PICO_DEFAULT_UART			1
// #define PICO_DEFAULT_UART_TX_PIN	20

#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
