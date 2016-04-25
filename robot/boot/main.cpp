#include <string.h>
#include <stdint.h>
#include "spi.h"

#include "uart.h"
#include "power.h"
#include "crc32.h"
#include "timer.h"
#include "recovery.h"
#include "MK02F12810.h"

// This is the remote entry point recovery mode
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
  MicroWait(100000);

  Power::init();

  // XXX-NDM: I had to do this, since the test fixture needs the Espressif to come up
  Power::enableEspressif();
  
  UART::init();
  EnterRecovery();

  // Tear down unnessessary components
  UART::shutdown();
  SPI::disable();

  SCB->VTOR = (uint32_t) IMAGE_HEADER->vector_tbl;
  IMAGE_HEADER->entry_point();
}
