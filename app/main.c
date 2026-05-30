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
#include "rtc.h"
#include "touchpad.h"
#include "pi.h"
#include "shared_i2c.h"
#ifdef PIN_VIBRO_DRV
    #include "vibromotor.h"
#endif
#if defined(BLEPIS) || defined(SNOWDIVE)
    #include "peripherals.h"
#endif
#ifdef BLEPIS_V1
    #include "mcp23017.h"
#endif
#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    #include "xl9535.h"
#endif
#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    #include "usbc.h"
#endif

// https://github.com/micropython/micropython/blob/5114f2c1ea7c05fc7ab920299967595cfc5307de/ports/rp2/modmachine.c#L179
// https://github.com/raspberrypi/pico-extras/issues/41
#include "pico/sleep.h"

volatile bool irq_fired = false;
// since the SDK doesn't support per-GPIO irq, we use this global irq and forward it
static void gpio_irq(uint gpio, uint32_t events)
{
	//printf("%s: gpio %d, events 0x%02X\r\n", __func__, gpio, events);
    irq_fired = true;
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
        xl9535_gpio_irq(gpio, events);
    #endif
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

	//sleep_ms(2000);

    dbg_light(urgb_u32(0xf*3, 0xf*3, 0));
	setup_shared_i2c();

    #ifndef NDEBUG
	    printf("rtc init\r\n");
    #endif

	rtc_init();
    rtc_set(125, 9, 4, 5, 33, 00);

    #ifndef NDEBUG
    i2c_inst_t* i2c_ec = get_shared_i2c_instance();
    i2c_scan(i2c_ec);
    //sleep_ms(2000);
    //i2c_scan(i2c_ec);
    #endif
	// For now, the `gpio` param is ignored and all enabled GPIOs generate the irq
	gpio_set_irq_enabled_with_callback(0xFF, 0, true, &gpio_irq);

    #ifdef BLEPIS_V1
        // MCP23017 is only used on Blepis v1
        #ifndef NDEBUG
	        printf("mcp init\r\n");
        #endif

    	mcp23017_init();
    #endif

    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
        // XL9535 is only used on Blepis v2 and Snowdive BTM_PALMTOP
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
    #ifdef PIN_VIBRO_DRV
    	vibromotor_init();
    #endif
    #if defined(BLEPIS) || defined(SNOWDIVE)
        peripherals_init();
    #endif

    dbg_light(urgb_u32(0, 0xf*3, 0xf*3));

    #ifndef NDEBUG
	    printf("puppet i2c temp init as controller\r\n");
    #endif

    setup_puppet_i2c_as_shared_i2c();
    #ifndef NDEBUG
    sleep_ms(2000);
    i2c_inst_t* i2c_cpu = get_puppet_shared_i2c_instance();
    //i2c_scan(i2c_cpu);
    #endif
    bool rtc_disabled = disable_i2c_rtc_clk();
    //sleep_ms(1000);

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

    //#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP) // needs cmakelists adjustment to work
    #if defined(SNOWDIVE_BTM_PALMTOP)
        #ifndef NDEBUG
	        printf("usbc init\r\n");
        #endif
        usbc_init();
    #endif

    bool exp_interrupts_disabled = false;

    if (rtc_disabled) {
        #ifndef NDEBUG
	        printf("xl9535 int init\r\n");
        #endif

        xl9535_enable_irq();
    } else {
        exp_interrupts_disabled = true;
        #ifndef NDEBUG
	        printf("RTC on, not enabling int\r\n");
        #endif
    }

    #ifndef NDEBUG
	    printf("ppt init\r\n");
    #endif

	puppet_i2c_init();

    #ifndef NDEBUG
	    printf("pipwr init\r\n");
    #endif

	pi_power_init();

	pi_power_on(POWER_ON_FW_INIT);

    dbg_light(urgb_u32(0x6*3, 0xd*3, 0xf*3)); // cyan
    //dbg_light(urgb_u32(0x5*3, 0xf*3, 0x5*3)); // light green


    #ifndef NDEBUG
    	printf("rtc year %d\r\n", rtc_get(REG_ID_RTC_YEAR));
	    printf("Starting main loop\r\n");
        uint16_t i = 0;
    #endif
    i2c_scan(i2c_ec);
    printf("hiiiii :3 \r\n"); // this was originally a debug statement but I'm leaving this in
    /*while (true) {
        if (xl9535_gpio_get(WUSB_RIGHT_INT) != false) {
            printf("WRN ");
        }
    }*/

	while (true) {
        #ifndef NDEBUG
        // constantly prints an incrementing counter, helping make sure that the code isn't stuck at some point
        i++;
        if (i % 10000 == 0) {
            printf("loop iter %d if %d\r\n", i, irq_fired);
            //printf("loop iter %d\r\n", i);
            printf("irq_sta %d %d\r\n", gpio_get(PIN_XL9535_TOP_INT), gpio_get(PIN_XL9535_BOTTOM_INT) );
            xl9535_debug();
        }
        if ((i % 100 == 0) && exp_interrupts_disabled) {
            //xl9535_poll_inputs(); // only needed for bottom expanders, at least
            /*if (i % 1000 == 0) {
                printf("polling %d\r\n", i);
            }*/
        }
        #endif
		__wfe();
	}

	return 0;
}
