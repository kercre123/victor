#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"
#include "dac.h"
#include "hal/i2c.h"
#include "hal/imu.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern int StartupSelfTest(void);

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
        I2CEnable();
        UartTransmit();
        TransmitDrop(buf, buflen, eof);
      }
    }
  }
}
void EnterRecoveryMode(void) {
  SCB->VTOR = 0;
  __asm { SVC 0 }
};

extern "C" void HardFault_Handler(void) {
  for(;;) ;
}

extern "C" const int __ESPRESSIF_SERIAL_NUMBER;

int main (void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Power up all ports
  SIM_SCGC5 |=
    SIM_SCGC5_PORTA_MASK |
    SIM_SCGC5_PORTB_MASK |
    SIM_SCGC5_PORTC_MASK |
    SIM_SCGC5_PORTD_MASK |
    SIM_SCGC5_PORTE_MASK;

  DebugInit();
  TimerInit();
  PowerInit();
  DACInit();

  I2CInit();
  
  // Wait for Espressif to boot
  for (int i=0; i<2; ++i) {
    Anki::Cozmo::HAL::MicroWait(1000000);
  }

  // Switch to 10MHz external reference to enable 100MHz clock
  MCG_C1 &= ~MCG_C1_IREFS_MASK;
  // Wait for IREF to turn off
  while((MCG->S & MCG_S_IREFST_MASK)) ;
  // Wait for FLL to lock
  while((MCG->S & MCG_S_CLKST_MASK)) ;

  Anki::Cozmo::HAL::MicroWait(100000); // Because the FLL is lame

  //IMUInit();
  OLEDInit();
  SPIInit();

  CameraInit();
  UartInit(); // MUST HAPPEN AFTER CAMARA INIT HAPPENS, OTHERWISE UARD RX FIFO WILL LOCK

  // IT IS NOT SAFE TO CALL ANY HAL FUNCTIONS (NOT EVEN DebugPrintf) AFTER CameraInit()
  // So, we just loop around for now

  //StartupSelfTest();

  FacePrintf("Address: %2x", (uint8_t) __ESPRESSIF_SERIAL_NUMBER);

  for(;;) {
    // Wait for head body sync to occur
    WaitForSync() ;
  }
}
