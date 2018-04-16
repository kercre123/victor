#include <stdio.h>
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "hal_timer.h"
#include "otp.h"

static inline void board_init_optimized_(void)
{
  // system init
  SetWord16(CLK_AMBA_REG, 0x00);                 // set clocks (hclk and pclk ) 16MHz
  SetWord16(SET_FREEZE_REG,FRZ_WDOG);            // stop watch dog    
  SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);        // open pads
  SetBits16(SYS_CTRL_REG,DEBUGGER_ENABLE,1);     // open debugger
  SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);       // exit peripheral power down
  
  // Power up peripherals' power domain
  SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
  while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));
  
  //Init boost regulator control (NOTE: 1V-BAT logic level)
  GPIO_INIT_PIN(BOOST, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_1V );
  
  //UART
  GPIO_INIT_PIN(UTX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  
  //Accel power down
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
}

void console_write(char* s) {
  hal_uart_write(s);
}

const int console_len_max = 75;
static char console_buf[console_len_max + 1];
static int console_len = 0;

//return ptr to full line, when available
static inline char* console_process_char_(char c)
{
  switch(c)
  {
    case '\r': //Return, EOL
    case '\n':
      if(console_len) {
        console_buf[console_len] = '\0';
        console_len = 0;
        return console_buf;
      }
      break;
    
    default:
      if( c >= ' ' && c <= '~' ) //printable char set
      {
        if( console_len < console_len_max ) {
          console_buf[console_len++] = c;
        }
      }
      //else, ignore all other chars
      break;
  }
  return NULL;
}

int main(void)
{
  board_init_optimized_(); //board_init();
  hal_uart_init();
  
  while(1)
  {
    char *s = console_process_char_( hal_uart_getchar() );
    if( s )
      cmd_process( s );
  }
}
