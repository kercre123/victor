#include <stdint.h>
#include "board.h"
#include "datasheet.h"
#include "gpio.h"
#include "hal_led.h"

//------------------------------------------------  
//    Interface
//------------------------------------------------

void hal_led_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;  
  
  board_init(); //cfg pins
  hal_led_off();
  hal_led_power(0);
  
  init = 1;
}

void hal_led_power(bool on)
{
  if( on )
    GPIO_SET(BOOST);
  else
    GPIO_CLR(BOOST);
}

static uint8_t m_hal_led_is_off = 0;
void hal_led_off(void)
{
  if( !m_hal_led_is_off )
  {
    GPIO_SET(D0);
    GPIO_SET(D1);
    GPIO_SET(D2);
    GPIO_SET(D3);
    GPIO_SET(D4);
    GPIO_SET(D5);
    GPIO_SET(D6);
    GPIO_SET(D7);
    GPIO_SET(D8);
    GPIO_SET(D9);
    GPIO_SET(D10);
    GPIO_SET(D11);
  }
  m_hal_led_is_off = 1;
}

void hal_led_on(uint16_t n)
{
  hal_led_off();
  
  switch(n)
  {
    case HAL_LED_D1_RED: GPIO_CLR(D2); break;
    case HAL_LED_D1_GRN: GPIO_CLR(D1); break;
    case HAL_LED_D1_BLU: GPIO_CLR(D0); break;
    
    case HAL_LED_D2_RED: GPIO_CLR(D5); break;
    case HAL_LED_D2_GRN: GPIO_CLR(D4); break;
    case HAL_LED_D2_BLU: GPIO_CLR(D3); break;
    
    case HAL_LED_D3_RED: GPIO_CLR(D8); break;
    case HAL_LED_D3_GRN: GPIO_CLR(D7); break;
    case HAL_LED_D3_BLU: GPIO_CLR(D6); break;
    
    case HAL_LED_D4_RED: GPIO_CLR(D11); break;
    case HAL_LED_D4_GRN: GPIO_CLR(D10); break;
    case HAL_LED_D4_BLU: GPIO_CLR(D9); break;
    
    default: return;
  }
  
  m_hal_led_is_off = 0;
}

