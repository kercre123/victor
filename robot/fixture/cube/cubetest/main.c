#include <stdio.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "hal_led.h"
#include "hal_spi.h"
#include "hal_timer.h"
#include "leds.h"

int g_echo_on = 0; //set 1 to enable echo at boot [debug]

//------------------------------------------------  
//    manage...stuff
//------------------------------------------------

void console_manage(void)
{
  char* line = console_getline(NULL); //polls uart
  if( line && cmd_process(line) < 0 ) { //valid line, not a command
    if( !g_echo_on ) {
      console_write(line);
      console_write("\n");
    }
  }
}

void blink(void)
{
  static uint32_t Tstart = 0;
  static uint16_t ledbf = 0;
  
  if( hal_timer_elapsed_us(Tstart) > 250*1000 )
  {
    Tstart = hal_timer_get_ticks();
    
    ledbf <<= 1;
    if( !(ledbf & LED_BF_ALL) )
      ledbf = 1;
    
    leds_set( ledbf );
    hal_led_power(1); //on
  }
}

//------------------------------------------------  
//    main
//------------------------------------------------

int main (void)
{
  board_init();
  console_init( g_echo_on ); //hal_uart_init();
  if( g_echo_on ) {
    console_write("\n\n*********** CUBETEST ****************\n" __DATE__ __TIME__ "\n");
    console_write("build compatibility: HW " HWVERS "\n");
  }
  
  hal_timer_init();
  leds_init(); //hal_led_init();
  
  while(1)
  {
    console_manage();
    
    //DEBUG:
    //blink();
  }
}

