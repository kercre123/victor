#include <string.h>
#include <stdarg.h>
#include "board.h"
#include "datasheet.h"

//------------------------------------------------  
//    Board
//------------------------------------------------

void board_init(void)
{
  static int inited = 0;
  if( inited )
    return;
  
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
  
  // Init LEDs
  GPIO_INIT_PIN(D0,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  
  //SPI
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_CS,  OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  
  //UART
  GPIO_INIT_PIN(UTX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  
  //OTHER
  GPIO_INIT_PIN(NC_P01,   INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(NC_P12,   INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(NC_P13,   INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  
  inited = 1;
}

//------------------------------------------------  
//    Global
//------------------------------------------------
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char* snformat(char *s, size_t n, const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  int len = vsnprintf(s, n, format, argptr);
  va_end(argptr);
  return s;
}
