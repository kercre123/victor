#include <string.h>
#include <stdint.h>
#include "spi.h"

#include "uart.h"
#include "power.h"
#include "crc32.h"
#include "timer.h"
#include "recovery.h"
#include "MK02F12810.h"

// Setup a vector for entering recovery mode / OTA mode
// NOTE: THIS REQUIRES THAT THE UART BE CONFIGURED AND 
// THAT SPI BE SYNCED AND SET TO SLAVE MODE!

// ALL NON-CRITICIAL SYSTEMS SHOULD BE SHUT DOWN TO PREVENT
// THE SYSTEM FROM GOING NUCLEAR.

// NOTE: MEMORY WILL BE ZEROED AT THIS ENTRY POINT

extern "C" uint32_t __Vectors;

// This is the remote entry point recovery mode
extern "C" void OTA_EntryPoint (void) {
  // Simple startup routine
  SCB->VTOR = __Vectors;
  memset((void*)0x1FFFE000, 0, 0x4000);
  
  EnterRecovery(true);
  NVIC_SystemReset();
}

int main (void) {
  using namespace Anki::Cozmo::HAL;

  // Power up all ports
  SIM_SCGC5 |=
    SIM_SCGC5_PORTA_MASK |
    SIM_SCGC5_PORTB_MASK |
    SIM_SCGC5_PORTC_MASK |
    SIM_SCGC5_PORTD_MASK |
    SIM_SCGC5_PORTE_MASK;

  __disable_irq();
  TimerInit();
  MicroWait(1000);

  Power::init();
  
  UART::init();
  EnterRecovery(false);

  // Tear down unnessessary components
  UART::shutdown();
  SPI::disable();
  Power::disableEspressif();

  SCB->VTOR = (uint32_t) IMAGE_HEADER->vector_tbl;
  IMAGE_HEADER->entry_point();
}
