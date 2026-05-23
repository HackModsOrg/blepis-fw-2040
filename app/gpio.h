#include <pico/stdlib.h>

void uni_gpio_init(uint8_t gpio);
void uni_gpio_set_dir(uint8_t gpio, bool out);
void uni_gpio_get_dir(uint8_t gpio);
void uni_gpio_put(uint8_t gpio, bool value);
bool uni_gpio_get(uint8_t gpio);
