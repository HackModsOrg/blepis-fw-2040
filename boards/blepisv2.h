// The pins of the XL are numbered as if they are part of the RP so A0 is 30 , B0 is 38 and B7 is 46
#pragma once

#define USB_VID				0x1209
#define USB_PID				0xB182
#define USB_PRODUCT			"BBQ20KBD"

#define PIN_INT				15
#define PIN_BKL				25

#define PIN_SDA				18
#define PIN_SCL				23

#define PIN_TP_RESET		31
#define PIN_TP_MOTION		22
#define PIN_TP_SHUTDOWN		30

#define PIN_PUPPET_SDA		28
#define PIN_PUPPET_SCL		29

#define BLEPIS
#define BLEPIS_V2

#define XL9535_TOP_ADDR 0x24 // U27, addr range from 30 to 45 (inclusive)
#define XL9535_BOTTOM_ADDR 0x26 // U25, addr range from 46 to 61 (inclusive)
#define FUSB302_ADDR 0x22
// 4 TMP102 sensors onboard, 0x48/0x49/0x4a/0x4b
//                            U35/ U32/ U33/ U34
#define TEMP_BASE_ADDR 0x48


/** Beepy inherited pins **/
#define PIN_PI_PWR 35
#define PIN_PI_SHUTDOWN 40
/** Blepis uses NEOPIXEL
#define PIN_LED_R 20
#define PIN_LED_G 19
#define PIN_LED_B 17
**/
#define PIN_NEO_PIXEL 20

#define PIN_BAT_ADC 26

/** Blepis specific pins and config **/

// muxes
#define PIN_USB_MUX_SEL 57 // 0x51 //
#define PIN_FUSB_MUX_SEL 59 // 0x51 //

// peripherals
#define PIN_BUZZER 14
#define PIN_VIBRO_DRV 21 // 0x50 //
#define PIN_LCDBL_DRV 24 // BKL //
#define PIN_DISP_EXTIN 39 // 0x53 //
#define PIN_DISP_RST 41  // 0x53 //
#define PIN_CHG_DIS 48 // 0x52 //
#define PIN_CHG_PWR 46 // 0x52 //
#define PIN_5V_PGOOD 55 // 0x52 //

#define PIN_XL9535_TOP_INT  16 // EX-T_INT in schematic
#define PIN_XL9535_BOTTOM_INT  19 // EX-B_INT in schematic

/** Blepis v2 specific pins and config **/
#define PIN_UART_MUX_SEL 38
#define PIN_I2C_PU_PWR 32
#define PIN_BUTTON_4 36
#define PIN_BUTTON_5 37
#define PIN_BUTTON_3 45
#define PIN_BUTTON_2 44
#define PIN_BUTTON_1 43
#define PIN_RTC_INT 42
#define PIN_VBUS_DET 47
#define PIN_PROG_DET 49
#define PIN_CHG_DET 50
#define PIN_USB_MUX_EN 51
#define PIN_TAL 52
#define PIN_USBC_OUT_EN 53
#define PIN_USBC_IN_EN 54
#define PIN_5V_BOOST_EN 56
#define PIN_FUSB_INT 58
#define PIN_CLED_EN 60
#define PIN_USBC_OUT_PWR_EN 61

#define NUM_OF_ROWS			7
#define PINS_ROWS \
	13, \
	12, \
	11, \
	10, \
	9, \
	8, \
	7

#define NUM_OF_COLS			6
#define PINS_COLS \
	0,  \
	1,  \
	2, \
	6, \
	5, \
	4

#define NUM_OF_BTNS			1
#define POWER_BTN           3
#define PINS_BTNS \
	POWER_BTN, // COL7
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
