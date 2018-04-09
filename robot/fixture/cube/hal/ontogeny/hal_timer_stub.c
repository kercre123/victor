#include <stdint.h>
#include "hal_timer.h"

void hal_timer_init(void)
{
}

uint32_t hal_timer_get_ticks(void)
{
  return 0;
}

uint32_t hal_timer_elapsed_us(uint32_t ticks_base)
{
  return 0;
}

void hal_timer_wait(uint32_t us)
{
}

void hal_timer_register_callback( void(*callback)(void) )
{
}

void hal_timer_set_led_callback( void(*callback)(void), uint16_t period_us )
{
}

