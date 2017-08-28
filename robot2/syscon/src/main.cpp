#include "common.h"
#include "hardware.h"

#include "power.h"
#include "comms.h"
#include "contacts.h"
#include "timer.h"
#include "motors.h"
#include "encoders.h"
#include "opto.h"
#include "analog.h"
#include "lights.h"
#include "mics.h"
#include "touch.h"

//#define DISABLE_WDOG

void Main_Execution(void) {
  #ifndef DISABLE_WDOG
  // Kick watch dog when we enter our service routine
  IWDG->KR = 0xAAAA;
  #endif

  // Do our main execution loop
  Comms::tick();
  Motors::tick();
  Contacts::tick();
  Opto::tick();
  Analog::tick();
  Lights::tick();
  Touch::tick();
}

int main (void) {
  // Our vector table is in SRAM and DMA mapping
  SYSCFG->CFGR1 = SYSCFG_CFGR1_USART1RX_DMA_RMP
                | (SYSCFG_CFGR1_MEM_MODE_0 * 3)
                ;

  // Enable clocking to our perfs
  Power::enableClocking();
  Power::init();

  Mics::init();
  Contacts::init();
  Analog::init();
  Timer::init();
  Comms::init();
  Motors::init();
  Encoders::init();
  Lights::init();
  Touch::init();

  __enable_irq(); // Start firing interrupts

  // This is all driven by IRQ logic
  Opto::init();

  // Low priority interrupts are now our main execution
  for (;;) {
    __asm("WFI");
    Power::eject();
  }
}
