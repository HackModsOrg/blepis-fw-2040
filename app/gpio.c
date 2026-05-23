/*
/ GPIO functions for compatibility betwen Beepy/Blepis v1/Blepis v2
/ GPIO on Beepy only ever uses RP2040 GPIOs
/ GPIO on Blepis v1 uses RP2040 GPIOs and a single MCP23017 I2C GPIO expander
/ GPIO on Blepis v2 uses RP2040 GPIOs and two XL9535 I2C GPIO expanders
/ This file provides GPIO setup/read/write functions so that GPIO operations
/ can be portable between all three platforms.
*/

#include <pico/stdlib.h>
#include <stdio.h>
#ifdef BLEPIS_V1
    #include "mcp23017.h"
#endif
#if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    #include "xl9535.h"
#endif

#define RP2040_MAX_GPIO 29 // last gpio for RP2040

void uni_gpio_init(uint8_t gpio) {
    if (gpio <= RP2040_MAX_GPIO) {
        return gpio_init(gpio);
    }
    #ifdef BLEPIS_V1
    if (gpio > RP2040_MAX_GPIO) {
        // do nothing
        return;
    }
    #endif
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    if (gpio > RP2040_MAX_GPIO) {
        // do nothing
        return;
    }
    #endif
    return;
}

void uni_gpio_set_dir(uint8_t gpio, bool out) {
    if (gpio <= RP2040_MAX_GPIO) {
        gpio_set_dir(gpio, out);
        return;
    }
    #ifdef BLEPIS_V1
    if (gpio > RP2040_MAX_GPIO) {
        mcp23017_gpio_set_dir(gpio, out);
        return;
    }
    #endif
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    if (gpio > RP2040_MAX_GPIO) {
        xl9535_gpio_set_dir(gpio, out);
        return;
    }
    #endif
    return;
}

bool uni_gpio_get_dir(uint8_t gpio) {
    if (gpio <= RP2040_MAX_GPIO)
        return gpio_get_dir(gpio);
    /*
    #ifdef BLEPIS_V1
    if (gpio > RP2040_MAX_GPIO) {
        return mcp23017_gpio_get_dir(out);
    }
    #endif
    */
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    if (gpio > RP2040_MAX_GPIO) {
        // TODO XL9535 support
        //return xl9535_gpio_get_dir(gpio);
    }
    #endif
    return 0;
}

void uni_gpio_put(uint8_t gpio, bool value) {
    if (gpio <= RP2040_MAX_GPIO) {
        gpio_put(gpio, value);
        return;
    }
    #ifdef BLEPIS_V1
    if (gpio > RP2040_MAX_GPIO) {
        mcp23017_gpio_put(gpio, value);
        return;
    }
    #endif
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    if (gpio > RP2040_MAX_GPIO) {
        xl9535_gpio_put(gpio, value);
        return;
    }
    #endif
    return;
}

bool uni_gpio_get(uint8_t gpio) {
    if (gpio <= RP2040_MAX_GPIO) {
        return gpio_get(gpio);
    }
    #ifdef BLEPIS_V1
    if (gpio > RP2040_MAX_GPIO) {
        //return mcp23017_gpio_get(gpio); // unimplemented right now
        return 0;
    }
    #endif
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP)
    if (gpio > RP2040_MAX_GPIO) {
        return xl9535_gpio_get(gpio);
    }
    #endif
    return 0;
}
