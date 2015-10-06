#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"
#include "dac.h"
#include "hal/i2c.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

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
        //UartTransmit();
        // Send data, if we have any
        if (buflen && buflen < 120)
        {
            //TransmitDrop(buf, buflen, eof);
        }

        //UartReceive(); // This should be done last
      }
    }
  }
}

void hardware_init(void)
{
  SIM_SCGC5 |= 
    SIM_SCGC5_PORTA_MASK |
    SIM_SCGC5_PORTB_MASK |
    SIM_SCGC5_PORTC_MASK |
    SIM_SCGC5_PORTD_MASK |
    SIM_SCGC5_PORTE_MASK;
}

int main (void)
{
  using namespace Anki::Cozmo::HAL;
  
  hardware_init();
  DebugInit();

  DebugPrintf("\r\nHeadboard 4.1 is booting.\n\n\r");

  TimerInit();
  PowerInit();
  I2CInit();

  DebugPrintf("Espressif startup time... ");
  for (int i=0; i<5; ++i) Anki::Cozmo::HAL::MicroWait(1000000);
  DebugPrintf("Done\n\r");
  
  // Switch to 10MHz external reference to enable 100MHz clock
  MCG_C2 &= ~MCG_C2_EREFS_MASK;
  
  //SPIInit();
  //DacInit();
  //i2c_init();
  //UartInit();
  
  CameraInit();
  for(;;) ;
}
