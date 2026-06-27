#include <pico/stdlib.h>

void uni_gpio_init(uint8_t gpio);
void uni_gpio_set_dir(uint8_t gpio, bool out);
void uni_gpio_put(uint8_t gpio, bool value);
void uni_gpio_pull_up(uint8_t gpio);
bool uni_gpio_get(uint8_t gpio);
bool uni_gpio_get_out_level(uint8_t gpio);
bool uni_gpio_get_dir(uint8_t gpio);
bool uni_gpio_is_dir_out(uint8_t gpio);

void process_gpio_update(uint8_t gpio, bool new_state);
