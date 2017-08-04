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

extern "C" {
  void __visitor_init() {
    // Enable clocking to our perfs
    Power::enableClocking();
    Power::init();

    Contacts::init();
    Analog::init();
  }

  void __visitor_stop() {
    // We cannot charge during DFU
    Contacts::stop();
    Analog::stop();
    Power::stop();
  }

  void __visitor_tick() {
    Contacts::tick();
    Analog::tick();
  }
}

void Main_Execution(void) {
  #ifndef DISABLE_WDOG
  // Kick watch dog when we enter our service routine
  IWDG->KR = 0xAAAA;
  WWDG->CR = 0xFF;  // Enabled with 64 ticks
  #endif

  // Do our main execution loop
  Comms::tick();
  Motors::tick();
  Opto::tick();
  Analog::tick();
  Contacts::tick();
  Lights::tick();
  Touch::tick();
}

int main (void) {
  // Our vector table is in SRAM and DMA mapping
  SYSCFG->CFGR1 = SYSCFG_CFGR1_USART1RX_DMA_RMP
                | (SYSCFG_CFGR1_MEM_MODE_0 * 3)
                ;

  //Mics::init(); // This does not work with the motors enabled
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
