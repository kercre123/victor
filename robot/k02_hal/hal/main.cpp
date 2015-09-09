// SDK Included Files - Please delete me!
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "fsl_debug_console.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"
#include "dac.h"
#include "hal/i2c.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void CameraInit(void);
      void TimerInit(void);
      void I2CInit(void);
    }
  }
}

// This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
void PowerInit()
{
  // I/Os used during startup
  GPIO_PIN_SOURCE(POWEREN, PTA, 2);
  GPIO_PIN_SOURCE(SCK, PTE, 17);
  GPIO_PIN_SOURCE(MOSI, PTE, 18);
  
  // Pull-down SCK during ESP8266 boot
  GPIO_IN(GPIO_SCK, PIN_SCK);
  SOURCE_SETUP(GPIO_SCK, SOURCE_SCK, SourceGPIO | SourcePullDown);    
  
  // Pull MOSI low to put ESP8266 into bootloader mode
  // XXX: Normally, you should drive this high and rely on the cable to enter debug mode!
  GPIO_IN(GPIO_MOSI, PIN_MOSI);
  SOURCE_SETUP(GPIO_MOSI, SOURCE_MOSI, SourceGPIO | SourcePullDown);  
  
  // Turn on 2v8 and 3v3 rails
  GPIO_SET(GPIO_POWEREN, PIN_POWEREN);
  GPIO_OUT(GPIO_POWEREN, PIN_POWEREN);
  SOURCE_SETUP(GPIO_POWEREN, SOURCE_POWEREN, SourceGPIO);    
}

int main (void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Initialize standard SDK demo application pins
  hardware_init();
  
  // Call this function to initialize the console UART. This function
  // enables the use of STDIO functions (printf, scanf, etc.)
  dbg_uart_init();

  PRINTF("\r\nTesting self-contained project file.\n\n\r");

  TimerInit();
  PowerInit();
  //I2CInit();
  //CameraInit();
  //dac_init();
  spi_init();
  //i2c_init();
  //uart_init();  
  
  for (;;) ;
}
