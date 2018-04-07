#ifndef HAL_LED_H_
#define HAL_LED_H_

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

#define HAL_NUM_LEDS  12

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

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
void hal_led_power(bool on); //1=enable 0=disable the VLED boost regulator
void hal_led_on(uint16_t n); //only 1 LED is on at a time. invalid/inactive leds are turned off
void hal_led_off(void); //all off

#endif //HAL_LED_H_

