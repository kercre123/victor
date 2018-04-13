#include <stdio.h>
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "hal_timer.h"
#include "otp.h"

const int g_echo_on = 0; //set 1 to enable echo at boot [debug]

//------------------------------------------------  
//    manage...stuff
//------------------------------------------------

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

//------------------------------------------------  
//    main
//------------------------------------------------

int main(void)
{
  /*/DEBUG:
  #warning "default to echo on for debug"
  g_echo_on = 1;
  //-*/
  
  board_init_optimized_(); //board_init();
  console_init( g_echo_on ); //hal_uart_init();
  //if( g_echo_on )
  //  console_write("\n\n*********** CUBE OTP ***********\n" __DATE__ " " __TIME__ "\n");
  
  hal_timer_init();
  
  while(1)
  {
    cmd_process( console_getline(NULL) ); //console_manage();
  }
}

