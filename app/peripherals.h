#pragma once

#include <stdint.h>

void peripherals_init(void);
void boost_enable(void);
void boost_disable(void);
void charger_enable(void);
void charger_disable(void);
void charger_lopwr(void);
void charger_hipwr(void);
void usbmux_hub(void);
void usbmux_host(void);
void usbmux_rp2040(void);
void fusbmux_rp2040(void);
void fusbmux_zero(void);
void uartmux_exp(void);
void uartmux_intl(void);
