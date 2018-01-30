#include <assert.h>
#include <stdint.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "board.h"
#include "common.h"
#include "hal_spi.h"

//BMA222E Datasheet, Timing:
//Fclk max 10MHz (Vddio > 1.62V)
//min SCK low/high pulse 20ns
//SDI setup/hold time 20ns (BMA222E)
//SDO output delay 40ns (Vddio > 2.4V)
//CSB setup/hold time 20/40ns
//min idle time between write accesses <-> normal,standby,low-pow-2 modes: 2us
//min idle time between write accessss <-> suspend,low-pow-1 modes: 450us

void hal_spi_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;

  board_init(); //cfg pins
  init = 1;
}

void hal_spi_set_cs(void) {
  GPIO_SET(ACC_CS);
  GPIO_PIN_FUNC(ACC_SDA, INPUT_PULLDOWN, PID_GPIO); //reset dat line to input
}

void hal_spi_clr_cs(void) {
  GPIO_CLR(ACC_CS);
}

void hal_spi_write(uint8_t dat)
{
  GPIO_PIN_FUNC(ACC_SDA, OUTPUT, PID_GPIO);
  
  for(int bit=0x80; bit > 0; bit >>= 1 )
  {
    GPIO_WRITE(ACC_SCK, 0); //mode 3: SDA change on falling edge
    GPIO_WRITE(ACC_SDA, dat & bit );
    GPIO_WRITE(ACC_SCK, 1); //mode 3, SDA sampled on rising edge
  }
  
  GPIO_WRITE(ACC_SDA, 0);
}

uint8_t hal_spi_read(void)
{
  uint8_t dat = 0;
  GPIO_PIN_FUNC(ACC_SDA, INPUT_PULLDOWN, PID_GPIO); //reset dat line to input
  
  for(int x=0; x<8; x++)
  {
    dat <<= 1;
    GPIO_WRITE(ACC_SCK, 0); //mode 3: SDA change on falling edge
    dat |= GPIO_READ(ACC_SDA);
    GPIO_WRITE(ACC_SCK, 1); //mode 3, SDA sampled on rising edge
  }
  
  return dat;
}

