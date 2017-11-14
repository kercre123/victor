#include <stdint.h>

#include "board.h"

extern void (*ble_send)(uint8_t length, const void* data);

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

void hal_led_power(bool on)
{
  if( on )
    GPIO_SET(BOOST_EN);
  else
    GPIO_CLR(BOOST_EN);
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

int main(void) {
  static const uint8_t BOOT_MESSAGE[] = "hello!";
  ble_send(sizeof(BOOT_MESSAGE), BOOT_MESSAGE);

  board_init(); //cfg pins
  hal_led_off();
  hal_led_power(1);
}

void deinit(void) {
  hal_led_off();
  hal_led_power(0);
}

void tick(void) {
  static int countup = 0;
  static int led = 0;
  
  if (countup++ < 10) return ;
  hal_led_on(led++);
  
  if (led >= 12) led = 0;
  countup = 0;
}

void recv(uint8_t length, const void* data) {
  ble_send(length, data);
}
