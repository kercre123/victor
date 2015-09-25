#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
      // Import init functions from all HAL components
      void CameraInit(void);
      void TimerInit(void);
      void I2CInit(void);
      void PowerInit(void);
      
      // This method is called at 7.5KHz (once per scan line)
      // After 7,680 (core) cycles, it is illegal to run any DMA or take any interrupt
      // So, you must hit all the registers up front in this method, and set up any DMA to finish quickly
      void HALExec(u8* buf, int buflen, int eof)
      {
        // Send data, if we have any
        if (buflen && buflen < 120)
        {
            TransmitDrop(buf, buflen, eof);
        }
      }
    }
  }
}

int main (void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Kill this off as soon as we can ditch the Freescale libs
  hardware_init();
  dbg_uart_init();

  PRINTF("\r\nTesting self-contained project file.\n\n\r");

  TimerInit();
  PowerInit();
  I2CInit();

  // Espressif startup time
	for (int i=0; i<5; ++i) Anki::Cozmo::HAL::MicroWait(1000000);
  
  SPIInit();
  //dac_init();
  //i2c_init();
  //uart_init();
  
  CameraInit();
  for(;;) ;
}
