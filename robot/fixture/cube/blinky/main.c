#include <stdio.h>
#include "board.h"
#include "hal_led.h"
#include "hal_timer.h"

int main (void)
{
  board_init();
  hal_timer_init();
  hal_led_init();
  //hal_uart_init();
  //hal_uart_printf("\n\n*********** BLINKY ****************\n%s %s\n", __DATE__, __TIME__);
  
  hal_led_power(1);
  while(1)
  {
    for(int n=0; n<12; n++)
    {
      uint32_t Tstart = hal_timer_get_ticks();
      while( hal_timer_elapsed_us(Tstart) < 250*1000 ) {
        hal_led_on(n);
        hal_timer_wait(700);
        hal_led_off();
        hal_timer_wait(11*700);
      }
    }
  }
}

