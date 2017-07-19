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
  // Do our main execution loop
  Comms::tick();
  Motors::tick();
  Opto::tick();
  Analog::tick();
  Contacts::tick();
}

int main (void) {
  // Our vector table is in SRAM and DMA mapping
  SYSCFG->CFGR1 = SYSCFG_CFGR1_USART1RX_DMA_RMP
                | (SYSCFG_CFGR1_MEM_MODE_0 * 3)
                ;

  Timer::init();
  Comms::init();
  Motors::init();
  Encoders::init();

  __enable_irq(); // Start firing interrupts

  // This is all driven by IRQ logic
  Opto::init();

  // Low priority interrupts are now our main execution
  for (;;) {
    __asm("WFI");
    Power::eject();
  }
}
