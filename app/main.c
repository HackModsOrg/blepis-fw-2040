#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>

#include <hardware/rtc.h>

#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

#include "backlight.h"
#include "debug.h"
#ifdef BEEPY
    #include "gpioexp.h"
#endif
#include "interrupt.h"
#include "keyboard.h"
#include "puppet_i2c.h"
#include "usb.h"
#include "reg.h"
#include "touchpad.h"
#include "pi.h"
#include "shared_i2c.h"
#ifdef BLEPIS
    #include "vibromotor.h"
    #include "peripherals.h"
#endif
#ifdef BLEPIS_V1
    #include "mcp23017.h"
#endif
#ifdef BLEPIS_V2
    #include "xl9535.h"
#endif

// https://github.com/micropython/micropython/blob/5114f2c1ea7c05fc7ab920299967595cfc5307de/ports/rp2/modmachine.c#L179
// https://github.com/raspberrypi/pico-extras/issues/41
#include "pico/sleep.h"

// since the SDK doesn't support per-GPIO irq, we use this global irq and forward it
static void gpio_irq(uint gpio, uint32_t events)
{
//	printf("%s: gpio %d, events 0x%02X\r\n", __func__, gpio, events);
	touchpad_gpio_irq(gpio, events);
    #ifdef BEEPY
    	gpioexp_gpio_irq(gpio, events);
    #endif
}

// TODO: Microphone
int main(void)
{
	// This order is important because it determines callback call order
    // there's stil things to reshuffle, but do take care

    usb_init();

    #ifndef NDEBUG
    	printf("led init\r\n");
    #endif

	led_init();

    dbg_light(urgb_u32(0xf*3, 0, 0));

    #ifndef NDEBUG
	    debug_init();
    #endif

	sleep_ms(2000);

    dbg_light(urgb_u32(0xf*3, 0xf*3, 0));
	setup_shared_i2c();

    #ifndef NDEBUG
	    printf("rtc init\r\n");
    #endif

	rtc_init();

    #ifdef BLEPIS_V1
        // MCP23017 is only used on Blepis v1
        #ifndef NDEBUG
	        printf("mcp init\r\n");
        #endif

    	mcp23017_init();
    #endif

    #ifdef BLEPIS_V2
        // XL9535 is only used on Blepis v2
        #ifndef NDEBUG
	        printf("xl9535 init\r\n");
        #endif

    	bool xl_found = xl9535_init();
        // this mechanism is temporarily disabled because it doesn't work right now
        // maybe? the xl_detect code needs to be rewritten to use register reads?
        /*
        if (!xl_found) { 
            #ifndef NDEBUG
            printf("one of xl9535 is not found, cannot boot\r\n");
            #endif
            while (true) {
                // cannot proceed, so we just blink
                dbg_light(urgb_u32(0xf*3, 0, 0));
            	sleep_ms(500);
                dbg_light(urgb_u32(0, 0, 0));
            	sleep_ms(500);
            }
        } */

    #endif

    dbg_light(urgb_u32(0, 0xf*3, 0));
    //dbg_light(urgb_u32(0xf*5, 0xb*5, 0xc*5));

    #ifndef NDEBUG
	    printf("reg init\r\n");
    #endif

	reg_init();

    #ifndef NDEBUG
	    printf("peripheral init\r\n");
    #endif

	backlight_init();
    #ifdef BLEPIS
    	vibromotor_init();
        peripherals_init();
    #endif

    dbg_light(urgb_u32(0, 0xf*3, 0xf*3));

    // gpioexp only works on OG beepy so far
    #ifdef BEEPY
        #ifndef NDEBUG
	        printf("gpioexp init\r\n");
        #endif
    	gpioexp_init();
    #endif

    #ifndef NDEBUG
	    printf("keeb init\r\n");
    #endif

	keyboard_init();

	// For now, the `gpio` param is ignored and all enabled GPIOs generate the irq
	gpio_set_irq_enabled_with_callback(0xFF, 0, true, &gpio_irq);

    dbg_light(urgb_u32(0xf*3, 0, 0xf*3));

    #ifndef NDEBUG
	    printf("touch init\r\n");
    #endif

	bool touch_found = touchpad_init();
    if (!touch_found) {
        #ifndef NDEBUG
        printf("touchpad not found\r\n");
        #endif
        dbg_light(urgb_u32(0xf*3, 0, 0));
    	sleep_ms(500);
    }

    #ifndef NDEBUG
	    printf("int init\r\n");
    #endif

	interrupt_init();

    #ifndef NDEBUG
	    printf("ppt init\r\n");
    #endif

	puppet_i2c_init();

    #ifndef NDEBUG
	    printf("pipwr init\r\n");
    #endif

	pi_power_init();

	pi_power_on(POWER_ON_FW_INIT);

    dbg_light(urgb_u32(0, 0, 0xf*3));

    #ifndef NDEBUG
	    printf("Starting main loop\r\n");
        uint16_t i = 0;
    #endif

	while (true) {
        #ifndef NDEBUG
        // constantly prints an incrementing counter, helping make sure that the code isn't stuck at some point
        i++;
        if (i % 10000 == 0) {
            printf("loop iter %d\r\n", i);
        }
        #endif
		__wfe();
	}

	return 0;
}
