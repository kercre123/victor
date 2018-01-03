#ifndef __LIGHTS_H
#define __LIGHTS_H

#include "board.h"

enum led_e {
  HAL_LED_D1_RED = 0,
  HAL_LED_D1_GRN = 1,
  HAL_LED_D1_BLU = 2,
  HAL_LED_D2_RED = 3,
  HAL_LED_D2_GRN = 4,
  HAL_LED_D2_BLU = 5,
  HAL_LED_D3_RED = 6,
  HAL_LED_D3_GRN = 7,
  HAL_LED_D3_BLU = 8,
  HAL_LED_D4_RED = 9,
  HAL_LED_D4_GRN = 10,
  HAL_LED_D4_BLU = 11
};

void hal_led_init(void);
void hal_led_stop(void);
void hal_led_set(const uint8_t* colors);

#endif
