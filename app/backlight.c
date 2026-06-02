#include "backlight.h"
#include "reg.h"

#include <hardware/pwm.h>
#include <pico/stdlib.h>
#include <stdio.h>

void backlight_sync(void) {
  pwm_set_gpio_level(PIN_BKL, reg_get_value(REG_ID_BKL) * 0x80);
#ifdef PIN_LCDBL_DRV
  // the given LCD backlight level ranges from 0 to 255 and needs
  // to scale from 0 to (2^16)-1 (max uint16_t), so the scaling factor
  // required is:
  // (2^16)-1 / 255 = 257 = 0x101
  uint16_t lcdbl_drv = reg_get_value(REG_ID_BK2) * 0x101;
  printf("set lcdbl_drv=%d\n", lcdbl_drv);
  pwm_set_gpio_level(PIN_LCDBL_DRV, lcdbl_drv);
#endif
}

void backlight_init(void) {
  gpio_set_function(PIN_BKL, GPIO_FUNC_PWM);

  const uint slice_num = pwm_gpio_to_slice_num(PIN_BKL);

  pwm_config config = pwm_get_default_config();
  pwm_init(slice_num, &config, true);

#ifdef PIN_LCDBL_DRV
  // display

  gpio_set_function(PIN_LCDBL_DRV, GPIO_FUNC_PWM);

  const uint slice_num2 = pwm_gpio_to_slice_num(PIN_LCDBL_DRV);
  pwm_config config2 = pwm_get_default_config();
  pwm_init(slice_num2, &config2, true);
#endif

  backlight_sync();
}
