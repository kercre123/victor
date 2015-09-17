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
          // Temporarily disable current transfer
          DMA_ERQ = 0;
          
          // I happen to know I can access -1 and -2 (the caller left space for me)
          buf[-2] = 0xA5;                      // UART header
          buf[-1] = buflen | (eof ? 0x80 : 0); // Set length, plus flag indicating end of frame
          
          // Kick off DMA transfer
          DMA_TCD1_SADDR = (uint32_t)(buf-2);// Offset -2 to include the header
          DMA_TCD1_CITER_ELINKNO = buflen+2; // Current major loop iteration
          DMA_TCD1_BITER_ELINKNO = buflen+2; // Beginning major loop iteration
          DMA_TCD1_CSR = BM_DMA_TCDn_CSR_DREQ;  // Stop channel and set to end on last major loop iteration

          DMA_ERQ = DMA_ERQ_ERQ1_MASK | DMA_ERQ_ERQ0_MASK;
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
  //I2CInit();
  
  //dac_init();
  //spi_init();  
  i2c_init();
  //uart_init();  
  
  //CameraInit();
  for (;;) ;
}
