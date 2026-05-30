/*
Blepis-only file
*/

#include "reg.h"
#include "gpio.h"
#include "peripherals.h"

#include <stdio.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

alarm_id_t g_extcomin_alarm = -1;
uint32_t extcomin_alarm_ms = 500;

static int64_t extcomin_alarm_callback(alarm_id_t _, void* __);

void init_unused_pins(void) {
    #ifdef PIN_UNUSED1
    uni_gpio_set_dir(PIN_UNUSED1, GPIO_OUT);
    #endif
    #ifdef PIN_UNUSED2
    uni_gpio_set_dir(PIN_UNUSED3, GPIO_OUT);
    #endif
    #ifdef PIN_UNUSED3
    uni_gpio_set_dir(PIN_UNUSED3, GPIO_OUT);
    #endif
    #ifdef PIN_UNUSED4
    uni_gpio_set_dir(PIN_UNUSED4, GPIO_OUT);
    #endif
    #ifdef PIN_UNUSED5
    uni_gpio_set_dir(PIN_UNUSED5, GPIO_OUT);
    #endif
}

void peripherals_init(void)
{
    // charging pins
	//uni_gpio_set_dir(PIN_CHG_DIS, GPIO_OUT);
    //charger_enable();
	//uni_gpio_set_dir(PIN_CHG_PWR, GPIO_OUT);
    //charger_lopwr();
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    // 5v boost
	uni_gpio_set_dir(PIN_5V_BOOST_EN, GPIO_OUT);
    boost_disable();
    // uart mux
    uartmux_exp(); // set the value first to avoid disconnecting the CPU in the middle of debug UART comms
	uni_gpio_set_dir(PIN_UART_MUX_SEL, GPIO_OUT);
    #endif
    #ifdef BLEPIS_V2
    // charging LED enable - hard-wired to high for now
    // afaict needs to be high for the SD card LED to light up
	uni_gpio_set_dir(PIN_CLED_EN, GPIO_OUT);
	uni_gpio_put(PIN_CLED_EN, 1);
    #endif
    // usb and fusb muxes
    #ifdef BLEPIS_V2
    usbmux_rp2040();
	uni_gpio_set_dir(PIN_USB_MUX_SEL, GPIO_OUT);
    #endif
    #ifdef SNOWDIVE_BTM_PALMTOP
    usbmux_hub();
	uni_gpio_set_dir(PIN_USB_MUX_TOP_C_SEL, GPIO_OUT);
	uni_gpio_set_dir(PIN_USB_MUX_TOP_S_SEL, GPIO_OUT);
    uni_gpio_put(PIN_USB_MUX_BTM_C_SEL, 0);
    uni_gpio_put(PIN_USB_MUX_BTM_S_SEL, 1);
	uni_gpio_set_dir(PIN_USB_MUX_BTM_C_SEL, GPIO_OUT);
	uni_gpio_set_dir(PIN_USB_MUX_BTM_S_SEL, GPIO_OUT);
    #endif
    init_unused_pins(); // setting unused pins to low preemptively so that they don't cause an interrupt storm on our simple IO expanders
    // setting FUSB mux SEL to out before setting it high means FUSB would momentarily disappear from the bus.
    // however, on stock blepis v1, this means Zero and 2040 I2C buses getting short-circuit, which, is pretty bad and undesirable
    // which is why here I set value first and then init.
    fusbmux_rp2040();
	uni_gpio_set_dir(PIN_FUSB_MUX_SEL, GPIO_OUT);
    // extin
    #if defined(BLEPIS) || defined(SNOWDIVE_BTM_PALMTOP)
    uni_gpio_put(PIN_DISP_EXTIN, 0);
    uni_gpio_set_dir(PIN_DISP_EXTIN, GPIO_OUT);
    g_extcomin_alarm = 1;
    (void)extcomin_alarm_callback(0, NULL);
    #endif
    #if defined(SNOWDIVE_BTM_PALMTOP)
    uni_gpio_put(PIN_IO_MUX_SEL, 0);
    uni_gpio_set_dir(PIN_IO_MUX_SEL, GPIO_OUT);
    #endif
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    uni_gpio_put(PIN_I2C_PU_PWR, 1);
    uni_gpio_set_dir(PIN_I2C_PU_PWR, GPIO_OUT);
    #endif
    #if defined(SNOWDIVE_BTM_PALMTOP)
    uni_gpio_put(PIN_VBUS_BYPASS, 1);
    uni_gpio_set_dir(PIN_VBUS_BYPASS, GPIO_OUT);
    uni_gpio_put(PIN_VINB_EN, 1);
    uni_gpio_set_dir(PIN_VINB_EN, GPIO_OUT);
    //sleep_ms(1000);
    //uni_gpio_put(PIN_VINB_PGOOD, 1);
    //uni_gpio_set_dir(PIN_VINB_PGOOD, GPIO_OUT);
    // Now, top USB-C power sink-source pins, default to "source"
    uni_gpio_put(PIN_USBC_IN_EN, 1);
    uni_gpio_put(PIN_USBC_OUT_EN, 0);
    uni_gpio_set_dir(PIN_USBC_IN_EN, GPIO_OUT);
    uni_gpio_set_dir(PIN_USBC_OUT_EN, GPIO_OUT);
    #endif
}

#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
void boost_enable()
{
    //printf("boost en\r\n");
	uni_gpio_put(PIN_5V_BOOST_EN, 1);
}

void boost_disable()
{
    //printf("boost dis\r\n");
	uni_gpio_put(PIN_5V_BOOST_EN, 0);
}

#endif

void charger_enable()
{
    //printf("chg en\r\n");
	uni_gpio_put(PIN_CHG_DIS, 0);
}

void charger_disable()
{
    //printf("chg dis\r\n");
	uni_gpio_put(PIN_CHG_DIS, 1);
}


void charger_lopwr()
{
    //printf("chg lpwr\r\n");
	uni_gpio_put(PIN_CHG_PWR, 0);
}

void charger_hipwr()
{
    //printf("chg hpwr\r\n");
	uni_gpio_put(PIN_CHG_PWR, 1);
}


void usbmux_host()
{
    //printf("usmbux host\r\n");
    #if defined(BLEPIS_V2)
	uni_gpio_put(PIN_USB_MUX_SEL, 0);
    #elif defined(SNOWDIVE_BTM_PALMTOP)
	uni_gpio_put(PIN_USB_MUX_TOP_C_SEL, 0);
	uni_gpio_put(PIN_USB_MUX_TOP_S_SEL, 1);
    #endif
}

void usbmux_hub()
{
    //printf("usmbux hub\r\n");
    #if defined(SNOWDIVE_BTM_PALMTOP)
	uni_gpio_put(PIN_USB_MUX_TOP_C_SEL, 1);
	uni_gpio_put(PIN_USB_MUX_TOP_S_SEL, 0);
    #endif
}

void usbmux_rp2040()
{
    //printf("usmbux rp2040\r\n");
    #if defined(BLEPIS_V2)
	uni_gpio_put(PIN_USB_MUX_SEL, 1); // mux polarity flipped on v2
    #elif defined(SNOWDIVE_BTM_PALMTOP)
    // no such thing
    #else
	uni_gpio_put(PIN_USB_MUX_SEL, 0);
    #endif
}

void fusbmux_rp2040()
{
    //printf("fusmbux en\r\n");
	uni_gpio_put(PIN_FUSB_MUX_SEL, 1);
}

void fusbmux_zero()
{
    //printf("fusmbux dis\r\n");
	uni_gpio_put(PIN_FUSB_MUX_SEL, 0);
}

#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)

// switches uart to external pin header, default
void uartmux_exp()
{
    //printf("uartmux exp\r\n");
    #ifdef SNOWDIVE_BTM_PALMTOP
	uni_gpio_put(PIN_UART_MUX_SEL, 0);
    #else
	uni_gpio_put(PIN_UART_MUX_SEL, 1);
    #endif
}

// switches uart to internal two pads (or QWIIC in case of Snowdive)
void uartmux_intl()
{
    //printf("uartmux intl\r\n");
    #ifdef SNOWDIVE_BTM_PALMTOP
	uni_gpio_put(PIN_UART_MUX_SEL, 1);
    #else
	uni_gpio_put(PIN_UART_MUX_SEL, 0);
    #endif
}

#endif

// EXTCOMIN driver

static int64_t extcomin_alarm_callback(alarm_id_t _, void* __)
{
    static bool extcomin_high = false;

    // Extcomin output disabled?
    if (g_extcomin_alarm < 0) {
        return 0;
    }

	uni_gpio_put(PIN_DISP_EXTIN, extcomin_high);
    extcomin_high = !extcomin_high;

    g_extcomin_alarm = add_alarm_in_ms(extcomin_alarm_ms, extcomin_alarm_callback, NULL, true);
    return 0;
}
