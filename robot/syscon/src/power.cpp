#include "common.h"
#include "hardware.h"

#include "comms.h"
#include "power.h"
#include "analog.h"
#include "vectors.h"
#include "flash.h"
#include "motors.h"
#include "encoders.h"
#include "opto.h"
#include "mics.h"
#include "lights.h"

extern "C" void SoftReset();

static const uint32_t APB1_CLOCKS = 0
              | RCC_APB1ENR_USART2EN
              | RCC_APB1ENR_TIM3EN
              | RCC_APB1ENR_TIM6EN
              | RCC_APB1ENR_TIM14EN
              | RCC_APB1ENR_I2C2EN
              | RCC_APB1ENR_SPI2EN
              ;

static const uint32_t APB2_CLOCKS = 0
              | RCC_APB2ENR_USART1EN
              | RCC_APB2ENR_TIM1EN
              | RCC_APB2ENR_TIM15EN
              | RCC_APB2ENR_TIM16EN
              | RCC_APB2ENR_TIM17EN
              | RCC_APB2ENR_SPI1EN
              | RCC_APB2ENR_SYSCFGEN
              | RCC_APB2ENR_ADC1EN
              ;

static PowerMode currentState = POWER_UNINIT;
static PowerMode desiredState = POWER_CALM;

static void enterBootloader(void);

void Power::init(void) {
  DFU_FLAG = 0;
  RCC->APB1ENR |= APB1_CLOCKS;
  RCC->APB2ENR |= APB2_CLOCKS;
  
  Power::adjustHead(true);
}

static inline void enableHead(void) {
  MAIN_EN::set();
  Mics::start();
  Lights::enable();
}

static inline void disableHead(void) {
  MAIN_EN::reset();
  Mics::stop();
  Lights::disable();
  Comms::reset();
}

static void markForErase(void) {
  // Mark the flash application space for deletion
  for (int i = 0; i < MAX_FAULT_COUNT; i++) {
    Flash::writeFaultReason(FAULT_USER_WIPE);
  }
}

static void enterBootloader(void) {
  // Shut down the motors
  Motors::stop();

  // Power down accessessories
  nVENC_EN::set();

  // Disable our DMA channels
  DMA1_Channel1->CCR = 0;
  DMA1_Channel2->CCR = 0;
  DMA1_Channel3->CCR = 0;
  DMA1_Channel4->CCR = 0;
  DMA1_Channel5->CCR = 0;

  // Reset all the perfs (including ones we are not using)
  RCC->APB1RSTR = 0
    | RCC_APB1RSTR_TIM2RST
    | RCC_APB1RSTR_TIM3RST
    | RCC_APB1RSTR_TIM6RST
    | RCC_APB1RSTR_TIM7RST
    | RCC_APB1RSTR_TIM14RST
    | RCC_APB1RSTR_SPI2RST
    | RCC_APB1RSTR_USART2RST
    | RCC_APB1RSTR_USART3RST
    | RCC_APB1RSTR_USART4RST
    | RCC_APB1RSTR_USART5RST
    | RCC_APB1RSTR_I2C1RST
    | RCC_APB1RSTR_I2C2RST
    | RCC_APB1RSTR_USBRST
    | RCC_APB1RSTR_CANRST
    | RCC_APB1RSTR_CRSRST
    | RCC_APB1RSTR_PWRRST
    | RCC_APB1RSTR_DACRST
    | RCC_APB1RSTR_CECRST
    ;

  RCC->APB2RSTR = 0
    | RCC_APB2RSTR_SYSCFGRST
    | RCC_APB2RSTR_ADCRST
    | RCC_APB2RSTR_USART8RST
    | RCC_APB2RSTR_USART7RST
    | RCC_APB2RSTR_USART6RST
    | RCC_APB2RSTR_TIM1RST
    | RCC_APB2RSTR_SPI1RST
    | RCC_APB2RSTR_USART1RST
    | RCC_APB2RSTR_TIM15RST
    | RCC_APB2RSTR_TIM16RST
    | RCC_APB2RSTR_TIM17RST
    | RCC_APB2RSTR_DBGMCURST
    | RCC_APB2RSTR_ADC1RST
    ;

  __asm("nop\nnop\nnop\nnop\nnop");

  RCC->APB1RSTR = 0;
  RCC->APB2RSTR = 0;

  // Disable clocking to everything but the GPIO
  RCC->APB1ENR &= ~APB1_CLOCKS;
  RCC->APB2ENR &= ~APB2_CLOCKS;

  __disable_irq();

  // Set to flash handler
  SYSCFG->CFGR1 = 0;

  markForErase();

  // Pass control back to the reset handler
  DFU_FLAG = DFU_ENTRY_POINT;
  NVIC->ICER[0] = ~0; // Disable all interrupts
  NVIC->ICPR[0] = ~0; // Clear all pending interrupts

  SoftReset();
}

void Power::wakeUp() {
  // Only wake up if we are in a low power state, not sleeping
  if (desiredState == POWER_CALM) {
    desiredState = POWER_ACTIVE;
  }
}

void Power::setMode(PowerMode set) {
  desiredState = set;
}

void Power::adjustHead(bool appStart) {
  static bool headPowered = false;  // head has power, but devices are not setup
  bool wantPower = desiredState != POWER_STOP;

  if (headPowered == wantPower) {
    return ;
  }

  if (wantPower) {
    if (!appStart) BODY_TX::mode(MODE_OUTPUT);
    enableHead();
  } else {
    disableHead();
  }

  headPowered = wantPower;
}

void Power::tick(void) {
  PowerMode desired = desiredState;

  if (currentState != desired) {
    // Power reduction code
    if (currentState == POWER_ACTIVE) {
      Opto::stop();
      Encoders::stop();
      Mics::reduce(true);
    } else if (desired == POWER_ACTIVE) {
      Encoders::init();
      Opto::start();
      Mics::reduce(false);
    } 

    if (desired == POWER_ERASE) {
      enterBootloader();
      return ;
    }

    currentState = desired;
  }

  switch (currentState) {
    case POWER_STOP:
      Analog::setPower(false);
      break ;
    default:
      Analog::setPower(true);
      break ;
  }
}
