#include "common.h"
#include "hardware.h"

#include "power.h"
#include "comms.h"
#include "contacts.h"
#include "timer.h"
#include "motors.h"
#include "encoders.h"
#include "i2c.h"
#include "opto.h"
#include "analog.h"
#include "lights.h"
#include "mics.h"
#include "touch.h"

static const int RESET_COUNT_MAX = 200 * 5; // 5 second timeout on main execution
static int reset_count = 0;

void Main_Execution(void) {
  // Do our main execution loop
  Comms::tick();
  Motors::tick();
  Contacts::tick();
  Opto::tick();
  Analog::tick();
  Lights::tick();
  Power::adjustHead();

  // Kick watch dog when we enter our service routine
  if (reset_count++ < RESET_COUNT_MAX) {
    IWDG->KR = 0xAAAA;
  }
}

int main (void) {
  // Our vector table is in SRAM and DMA mapping
  SYSCFG->CFGR1 = SYSCFG_CFGR1_USART1RX_DMA_RMP
                | (SYSCFG_CFGR1_MEM_MODE_0 * 3)
                ;

  // Create safe interrupt state
  NVIC->ICER[0]  = ~0;
  __enable_irq();

  Power::init();
  Mics::init();
  Analog::init();
  Contacts::init();
  Comms::init();
  Motors::init();
  Touch::init();
  I2C::init();
  Lights::init();
  Timer::init();

  // Low priority interrupts are now our main execution
  for (;;) {   
    Power::tick();
    __wfi();
    reset_count = 0;
  }
}
