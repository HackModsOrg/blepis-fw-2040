#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "shared_i2c.h"
#include "xl9535.h"
#include "gpio.h"

#define INP_REG 0x00 // output regs are 0x02, 0x03
#define OUT_REG 0x02 // output regs are 0x02, 0x03
#define DIR_REG 0x06 // config regs are 0x06, 0x07

static i2c_inst_t* i2c = NULL;

uint8_t get_expander_num(uint8_t gpio);
uint16_t xl_read_u16(uint8_t exp_addr, uint8_t reg);
void xl_write_u16(uint8_t exp_addr, uint8_t reg, uint16_t val);
void xl9535_gpio_update(uint8_t exp_num);
uint16_t gpio_bit_set(uint8_t gpio, uint8_t set, uint16_t value);
bool gpio_bit_get(uint16_t value, uint8_t gpio);
uint8_t get_bit_pos(uint8_t gpio);

/* Blepis v2 has two XL9535 GPIO expanders,
one top    (U27, 0x24), address range from 30 to 45 (incl)
one bottom (U25, 0x26), address range from 46 to 61 (incl)
*/

/* Snowdive v0 has two/four XL9535 GPIO expanders,
bottom board:
one left    (U21, 0x26), address range from 30 to 45 (incl)
one right   (U28, 0x27), address range from 46 to 61 (incl)
top board, when powered/connected:
one left    (U27, 0x24), address range from 62 to 77 (incl) ??
one right   (U25, 0x25), address range from 78 to 93 (incl) ??
*/

#ifdef BLEPIS_V2
#define EXPANDER_MAX_AMOUNT 2
uint8_t XL9535_ADDRS[EXPANDER_MAX_AMOUNT] = {XL9535_TOP_ADDR, XL9535_BOTTOM_ADDR};
#endif
#ifdef SNOWDIVE_BTM_PALMTOP_V0
#define EXPANDER_MAX_AMOUNT 4
uint8_t XL9535_ADDRS[EXPANDER_MAX_AMOUNT] = {XL9535_BTM_LEFT_ADDR, XL9535_BTM_RIGHT_ADDR, XL9535_TOP_LEFT_ADDR, XL9535_TOP_RIGHT_ADDR};
#endif

#define RP2040_MAX_GPIO 29 // last gpio possible for RP2040
// ofc this will have to be updated once we have RP2350 versions going on :3

static uint16_t IODIR[EXPANDER_MAX_AMOUNT];
static uint16_t GPIO[EXPANDER_MAX_AMOUNT];
static uint16_t INPUT[EXPANDER_MAX_AMOUNT];

bool xl9535_detect_from_to(int from, int to) {
    int ret, ret1, ret2;
    uint8_t reg = 0;
	uint8_t val[2];
    for(int exp_num=from; exp_num<to; exp_num++) {
        uint8_t exp_addr = XL9535_ADDRS[exp_num];

        shared_i2c_in_use = true;
        ret1 = i2c_write_blocking(i2c, exp_addr, &reg, sizeof(reg), true);
        ret2 = i2c_read_blocking(i2c, exp_addr, val, sizeof(val), false);
        shared_i2c_in_use = false;

        ret = ret1 | ret2;
        //printf("PROBE FAIL 0x%X 0x%X, %d %d\r\n", exp_addr, reg, ret1, ret2);
        if (ret < 1) {
            printf("PROBE FAIL 0x%X 0x%X, %d %d\r\n", exp_addr, reg, ret1, ret2);
            //#ifdef SNOWDIVE_BTM_PALMTOP_V0
            //    if (exp_num > 1) {
            //        // ignore, expanders 2 and 3 can be absent
            //    } else { return false; }
            //#else
            return false;
            //#endif
        }
    }
    return true;
}

bool xl9535_detect() {
    //uint8_t expanders_to_detect = EXPANDER_MAX_AMOUNT;
    return xl9535_detect_from_to(0, 2); // currently only two expanders are guaranteed to be present on either board
}

#ifdef SNOWDIVE_BTM_PALMTOP_V0
bool xl9535_detect_aux_expanders() {
    return xl9535_detect_from_to(2, 4); // expanders 2 and 3 are auxiliary on Snowdive boards
}
#endif

void xl9535_enable_irq(void) {
    // only set up interrupts well after all is well and done
    //gpio_set_irq_enabled(PIN_XL9535_TOP_INT, GPIO_IRQ_LEVEL_LOW, true);
    gpio_set_irq_enabled(PIN_XL9535_BOTTOM_INT, GPIO_IRQ_LEVEL_LOW, true);
}

void xl9535_disable_irq(void) {
    // only set up interrupts well after all is well and done
    //gpio_set_irq_enabled(PIN_XL9535_TOP_INT, GPIO_IRQ_LEVEL_LOW, false);
    gpio_set_irq_enabled(PIN_XL9535_BOTTOM_INT, GPIO_IRQ_LEVEL_LOW, false);
}

bool xl9535_init_main(void) {
    int ret;
    // "preliminary" init of variables
    for (int exp_num=0; exp_num<EXPANDER_MAX_AMOUNT; exp_num++) {
        IODIR[exp_num] = 0xffff; // Default: all pins are inputs
        GPIO[exp_num] = 0x0000; // Default: all outputs are off
        INPUT[exp_num] = 0xffff; // Default: all inputs are high
    }
    gpio_init(PIN_XL9535_TOP_INT);
    gpio_init(PIN_XL9535_BOTTOM_INT);
    gpio_set_dir(PIN_XL9535_TOP_INT, GPIO_IN);
    gpio_set_dir(PIN_XL9535_BOTTOM_INT, GPIO_IN);
    // interrupt GPIOs don't have pullups on our boards
    // and even if they do, it's safer this way
    // (even after we get interrupts, it'll also help once the top board is disconnected)
    gpio_pull_up(PIN_XL9535_TOP_INT);
    gpio_pull_up(PIN_XL9535_BOTTOM_INT);
    i2c = get_shared_i2c_instance();
    if (!xl9535_detect())
        return false;
    // populate the registers
    // two out of four reads will be failures on a standalone Snowdive keyboard, so let's just do two first:
    // both Blepis v2-v3 and Snowdive v0 require two expanders to be present
    uint8_t required_expanders = 2;
    for(int exp_num=0; exp_num<required_expanders; exp_num++) {
        uint8_t xl_addr = XL9535_ADDRS[exp_num];
        IODIR[exp_num] = xl_read_u16(xl_addr, DIR_REG);
        GPIO[exp_num] = xl_read_u16(xl_addr, OUT_REG);
        xl9535_gpio_update(exp_num);
    }
    #ifdef SNOWDIVE_BTM_PALMTOP
    // now, let's ensure access to the two extra GPIO expanders, by switching IOMUX_SEL
    xl9535_gpio_put(PIN_IO_MUX_SEL, 0);
    xl9535_gpio_set_dir(PIN_IO_MUX_SEL, GPIO_OUT);
    #endif

    #ifndef NDEBUG
        // some visual debug - disabling the charger IC and its associated LEDs
        //get_bit_pos(30);
        xl9535_gpio_set_dir(PIN_CHG_DIS, true);
        xl9535_gpio_put(PIN_CHG_DIS, true);
        sleep_ms(100);
        xl9535_gpio_put(PIN_CHG_DIS, false); // enabling charging
        //sleep_ms(2000);
    #endif
    /*
    This would be best placed into a separate function and fired off only after
    finding out that the PI_PWR signal has not been initialized yet
    */
    #ifdef SNOWDIVE_BTM_PALMTOP_V0
    // as-soon-as-possible values (especially relevant on snowdive v0
    //xl9535_gpio_set_dir(PIN_PI_PWR, GPIO_OUT);
    //xl9535_gpio_put(PIN_PI_PWR, 0);
    #endif
    return true;
}

#ifdef SNOWDIVE_BTM_PALMTOP
bool xl9535_init_aux(void) {
    if (xl9535_detect_aux_expanders()) {
        // only actually read default values into registers if the aux expanders are present!
        for(int exp_num=2; exp_num<EXPANDER_MAX_AMOUNT; exp_num++) {
            uint8_t xl_addr = XL9535_ADDRS[exp_num];
            IODIR[exp_num] = xl_read_u16(xl_addr, DIR_REG);
            GPIO[exp_num] = xl_read_u16(xl_addr, OUT_REG);
            xl9535_gpio_update(exp_num);
        }
        #ifdef SNOWDIVE_BTM_PALMTOP_V0
        // as-soon-as-possible values (especially relevant on snowdive v0
        xl9535_gpio_set_dir(PIN_ESP_PWR, GPIO_OUT);
        xl9535_gpio_set_dir(PIN_ESP_EN, GPIO_OUT);
        xl9535_gpio_put(PIN_ESP_PWR, 1);
        xl9535_gpio_put(PIN_ESP_EN, 0);
        sleep_ms(200);
        xl9535_gpio_put(PIN_ESP_EN, 1);
        #endif
        return true;
    } else {
        printf("aux expander detect fail\r\n");
        return false;
    }
}
#endif

uint16_t xl_read_u16(uint8_t exp_addr, uint8_t reg)
{
	uint8_t val[2];
    int ret, ret1, ret2;

    shared_i2c_in_use = true;
	ret1 = i2c_write_blocking(i2c, exp_addr, &reg, sizeof(reg), true);
	ret2 = i2c_read_blocking(i2c, exp_addr, val, sizeof(val), false);
    shared_i2c_in_use = false;

    ret = ret1 | ret2;
    if (ret < 1) {
        printf("FAIL 0x%X 0x%X, %d %d\r\n", exp_addr, reg, ret1, ret2);
        return 0;
    }

    uint16_t value = 0;
    value |= ( (uint16_t)val[1] ) << 8;
    value |= val[0];
    //printf("SUCC 0x%X 0x%X, (0x%X 0x%X 0x%X) %d %d\r\n", exp_addr, reg, val[0], val[1], value, ret1, ret2);
    return value;
}

void xl_write_u16(uint8_t exp_addr, uint8_t reg, uint16_t val)
{
	uint8_t buffer[3] = { reg, (uint8_t)(val), (uint8_t)(val >> 8)};
    shared_i2c_in_use = true;
	i2c_write_blocking(i2c, exp_addr, buffer, sizeof(buffer), false);
    shared_i2c_in_use = false;
}

volatile bool xl_irq_fired = false;

void xl9535_gpio_irq(uint8_t gpio, uint32_t events) {
    printf("irq_sta %d %d\r\n", gpio_get(PIN_XL9535_TOP_INT), gpio_get(PIN_XL9535_BOTTOM_INT) );
    if ((gpio != PIN_XL9535_TOP_INT) && (gpio != PIN_XL9535_BOTTOM_INT))
        return;
    //printf("%d ", events);
    if ( !(events & GPIO_IRQ_LEVEL_LOW) )
		return;
    xl_irq_fired = true;
    if (shared_i2c_in_use) {
        // oh no
        xl9535_disable_irq(); //
        // TODO SET FAILURE FLAG!!!
        return;
    }
    #ifdef BLEPIS_V2
    if (gpio == PIN_XL9535_TOP_INT)
        xl9535_gpio_update(0);
    if (gpio == PIN_XL9535_BOTTOM_INT)
        xl9535_gpio_update(1);
    #endif
    #ifdef SNOWDIVE_BTM_PALMTOP_V0
    // INTs are joined for top and bottom due to lack of pin-itis
    if (gpio == PIN_XL9535_BOTTOM_INT) {
        xl9535_gpio_update(0);
        xl9535_gpio_update(1);
    }
    if (gpio == PIN_XL9535_TOP_INT) {
        xl9535_gpio_update(2);
        xl9535_gpio_update(3);
    }
    #endif
}

void xl9535_poll_inputs(void) {
    #ifdef BLEPIS_V2
    //xl9535_gpio_update(0);
    xl9535_gpio_update(1);
    #endif
    #ifdef SNOWDIVE_BTM_PALMTOP_V0
    // INTs are joined for top and bottom due to lack of pin-itis
    xl9535_gpio_update(0);
    xl9535_gpio_update(1);
    // polling only needed for bottom expanders on snowdive rn
    /*
    xl9535_gpio_update(2);
    xl9535_gpio_update(3);
    */
    #endif
}

void process_port_update(uint8_t exp_num, uint16_t old_in, uint16_t new_in) {
    uint16_t toggled_pins = old_in^new_in;
    printf("NEW %d: 0x%X 0x%X 0x%X\r\n", exp_num, old_in, new_in, toggled_pins);
    for (int i=0;i<16;i++) { // going through all sixteen bits
        if (1 & (toggled_pins >> i)) { // this changed
            int gpio_num = RP2040_MAX_GPIO + 1 + exp_num*16 + i;
            bool new_state = (bool)( (new_in & (1 << i)) >> i );
            printf("GPIO %d changed to %d\r\n", gpio_num, new_state);
            process_gpio_update(gpio_num, new_state);
        }
        // a lot of fun code goes here
        //uint8_t exp_num = get_expander_num(gpio);
        // base gpio number for the expander
        //return gpio - start_gpio;
    }
}

void xl9535_gpio_update(uint8_t exp_num) {
    uint8_t xl_addr = XL9535_ADDRS[exp_num];
    uint16_t old_inputs = INPUT[exp_num];
    INPUT[exp_num] = xl_read_u16(xl_addr, INP_REG);
    #if defined(BLEPIS_V2) || defined(SNOWDIVE_BTM_PALMTOP_V0)
    // masking the dang rtc_int pin
    if (exp_num == get_expander_num(PIN_RTC_INT)) {
        uint16_t mask = ~(1 << get_bit_pos(PIN_RTC_INT));
        old_inputs &= mask;
        INPUT[exp_num] &= mask;
    }
    #endif
    if (old_inputs != INPUT[exp_num]) {
        process_port_update(exp_num, old_inputs, INPUT[exp_num]);
    }
}

/* Blepis v2 has two XL9535 GPIO expanders,
one top    (U27, 0x24), address range from 30 to 45 (incl)
one bottom (U25, 0x26), address range from 46 to 61 (incl)
*/

/* Snowdive v0 has two/four XL9535 GPIO expanders,
bottom board:
one left    (U21, 0x26), address range from 30 to 45 (incl)
one right   (U28, 0x27), address range from 46 to 61 (incl)
top board, when powered/connected:
one left    (U27, 0x24), address range from 62 to 77 (incl) ??
one right   (U25, 0x25), address range from 78 to 93 (incl) ??
*/

uint8_t get_expander_num(uint8_t gpio) {
    // TODO this algo will be rearranged depending on how GPIOs are mapped into registers
    // and how the reg uint16_t maps onto i2c bytes sent
    // TODO: check algo before running!!!
    uint8_t exp_number = EXPANDER_MAX_AMOUNT;
    uint8_t start_gpio, end_gpio;

    for (int i=0; i<EXPANDER_MAX_AMOUNT; i++) {
        start_gpio = RP2040_MAX_GPIO + 1 + i*16;
        end_gpio = start_gpio + 15;
        if (gpio >= start_gpio && gpio <= end_gpio) {
            exp_number = i; break;
        }
    }
    return exp_number;
}

uint8_t get_bit_pos(uint8_t gpio) {
    uint8_t exp_num = get_expander_num(gpio);
    // base gpio number for the expander
    uint8_t start_gpio = RP2040_MAX_GPIO + 1 + exp_num*16;
    return gpio - start_gpio;
}

void xl9535_gpio_put(uint8_t gpio, uint8_t value) {
    uint8_t exp_num = get_expander_num(gpio);
    uint8_t xl_addr = XL9535_ADDRS[exp_num];
    uint16_t gpio_val = gpio_bit_set(gpio, value, GPIO[exp_num]);
    xl_write_u16(xl_addr, OUT_REG, gpio_val);
    GPIO[exp_num] = gpio_val;
}

// TODO (wait what TODO, bruh i forgor)
bool xl9535_gpio_get(uint8_t gpio) {
    // this function reads directly from cached values, which only get updated when an interrupt happens
    uint8_t exp_num = get_expander_num(gpio);
    return gpio_bit_get(INPUT[exp_num], gpio);
}

bool xl9535_gpio_get_out_level(uint8_t gpio) {
    // useful for figuring out if a GPIO has been set to OUT already.
    // mostly for checking whether the Pi is already powered
    uint8_t exp_num = get_expander_num(gpio);
    return gpio_bit_get(GPIO[exp_num], gpio);
}

bool xl9535_gpio_is_dir_out(uint8_t gpio) {
    // useful for figuring out if a GPIO has been set to OUT already.
    // mostly for checking whether the Pi is already powered
    uint8_t exp_num = get_expander_num(gpio);
    return !gpio_bit_get(IODIR[exp_num], gpio);
}

void xl9535_gpio_set_dir(uint8_t gpio, uint8_t out) {
    uint8_t exp_num = get_expander_num(gpio);
    uint8_t xl_addr = XL9535_ADDRS[exp_num];
    uint16_t iodir_val = gpio_bit_set(gpio, !out, IODIR[exp_num]);
    xl_write_u16(xl_addr, DIR_REG, iodir_val);
    IODIR[exp_num] = iodir_val;
}

// TODO (once again, TODO WHAT???)
bool xl9535_gpio_get_dir(uint8_t gpio) {
    uint8_t exp_num = get_expander_num(gpio);
    return !gpio_bit_get(IODIR[exp_num], gpio);
}

uint16_t gpio_bit_set(uint8_t gpio, uint8_t set, uint16_t value) {
    if (set) { // set bit
        value = value | 1 << get_bit_pos(gpio);
    } else { // clear bit
        value = value & ~(1 << get_bit_pos(gpio));
    }
    return value;
}

bool gpio_bit_get(uint16_t value, uint8_t gpio) {
    uint8_t pos = get_bit_pos(gpio);
    return ( value >> pos & 1 ) & 0x1;
}

void xl9535_debug(void) {
    for(int exp_num=0; exp_num<EXPANDER_MAX_AMOUNT; exp_num++) {
    //for(int exp_num=0; exp_num<2; exp_num++) {
        //printf("xl9535 %d IRQ %d\r\n", exp_num, xl_irq_fired);
        printf("xl9535 %d OUT 0x%X\r\n", exp_num, GPIO[exp_num]);
        printf("xl9535 %d INP 0x%X\r\n", exp_num, INPUT[exp_num]);
        printf("xl9535 %d IOD 0x%X\r\n\r\n", exp_num, IODIR[exp_num]);
    }
}
