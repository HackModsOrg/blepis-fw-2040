#pragma once

bool xl9535_init();
bool xl9535_detect();

void xl9535_gpio_irq(uint8_t gpio, uint32_t events);
void xl9535_gpio_put(uint8_t gpio, uint8_t value);
void xl9535_gpio_set_dir(uint8_t gpio, uint8_t out);
void xl9535_gpio_pull_up(uint8_t gpio);
void xl9535_gpio_disable_pulls(uint8_t gpio);
