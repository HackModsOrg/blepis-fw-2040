/*
Blepis-only file
*/

#include "peripherals.h"
#include "gpio.h"
#include "reg.h"

#include <hardware/pwm.h>
#include <pico/stdlib.h>
#include <stdio.h>

static int64_t extcomin_start_after_initial_delay(alarm_id_t _, void *__);
static bool extcomin_repeating_timer(repeating_timer_t *rt);

void peripherals_init(void) {
  // charging pins
  uni_gpio_set_dir(PIN_CHG_DIS, GPIO_OUT);
  charger_enable();
  uni_gpio_set_dir(PIN_CHG_PWR, GPIO_OUT);
  charger_lopwr();

#ifdef BLEPIS_V2
  // 5v boost
  uni_gpio_set_dir(PIN_5V_BOOST_EN, GPIO_OUT);
  boost_disable();
  // uart mux
  uni_gpio_set_dir(PIN_UART_MUX_SEL, GPIO_OUT);
  uartmux_exp();
#endif

  // usb and fusb muxes
  usbmux_rp2040();
  uni_gpio_set_dir(PIN_USB_MUX_SEL, GPIO_OUT);

  // setting FUSB mux SEL to out before setting it high means FUSB
  // would momentarily disappear from the bus.
  // however, on stock blepis v1, this means Zero and 2040 I2C buses
  // getting short-circuit, which, is pretty bad and undesirable
  // which is why here I set value first and then init.
  fusbmux_rp2040();
  uni_gpio_set_dir(PIN_FUSB_MUX_SEL, GPIO_OUT);

  // extin
  // -> reproduce exact behaviour in sharp-drm driver
  uni_gpio_set_dir(PIN_DISP_EXTIN, GPIO_OUT);
  add_alarm_in_ms(500, extcomin_start_after_initial_delay, NULL, true);
}

#ifdef BLEPIS_V2
void boost_enable() {
  // printf("boost en\r\n");
  uni_gpio_put(PIN_5V_BOOST_EN, 1);
}

void boost_disable() {
  // printf("boost dis\r\n");
  uni_gpio_put(PIN_5V_BOOST_EN, 0);
}

#endif

void charger_enable() {
  // printf("chg en\r\n");
  uni_gpio_put(PIN_CHG_DIS, 0);
}

void charger_disable() {
  // printf("chg dis\r\n");
  uni_gpio_put(PIN_CHG_DIS, 1);
}

void charger_lopwr() {
  // printf("chg lpwr\r\n");
  uni_gpio_put(PIN_CHG_PWR, 0);
}

void charger_hipwr() {
  // printf("chg hpwr\r\n");
  uni_gpio_put(PIN_CHG_PWR, 1);
}

void usbmux_host() {
  // printf("usmbux host\r\n");
#ifdef BLEPIS_V2
  uni_gpio_put(PIN_USB_MUX_SEL, 0);
#else
  uni_gpio_put(PIN_USB_MUX_SEL, 1);
#endif
}

void usbmux_rp2040() {
  // printf("usmbux rp2040\r\n");
#ifdef BLEPIS_V2
  uni_gpio_put(PIN_USB_MUX_SEL, 1);
#else
  uni_gpio_put(PIN_USB_MUX_SEL, 0);
#endif
}

void fusbmux_rp2040() {
  // printf("fusmbux en\r\n");
  uni_gpio_put(PIN_FUSB_MUX_SEL, 1);
}

void fusbmux_zero() {
  // printf("fusmbux dis\r\n");
  uni_gpio_put(PIN_FUSB_MUX_SEL, 0);
}

#ifdef BLEPIS_V2

// switches uart to external pin header, default
void uartmux_exp() {
  // printf("uartmux exp\r\n");
  uni_gpio_put(PIN_UART_MUX_SEL, 1);
}

// switches uart to internal two pads
void uartmux_intl() {
  // printf("uartmux intl\r\n");
  uni_gpio_put(PIN_UART_MUX_SEL, 0);
}

#endif

// EXTCOMIN driver
static repeating_timer_t repeating_timer;

static int64_t extcomin_start_after_initial_delay(alarm_id_t _, void *__) {

  // run timer once after inital delay
  extcomin_repeating_timer(NULL);

  // schedule timer to be run with fixed delay from now on
  add_repeating_timer_ms(1000, extcomin_repeating_timer, NULL,
                         &repeating_timer);

  // do not run again
  return 0;
}

static bool extcomin_repeating_timer(repeating_timer_t *rt) {
  static bool extcomin_high = false;

  // toggle gpio
  extcomin_high = !extcomin_high;
  uni_gpio_put(PIN_DISP_EXTIN, extcomin_high);

  // run again
  return true;
}
