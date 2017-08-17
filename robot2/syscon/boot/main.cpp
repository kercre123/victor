#include "vectors.h"
#include "bignum.h"
#include "rsa_pss.h"
#include "flash.h"
#include "comms.h"

#include "stm32f0xx.h"

#include "power.h"
#include "analog.h"
#include "contacts.h"

#include "common.h"
#include "hardware.h"

//#define DISABLE_WDOG

extern "C" void StartApplication(const uint32_t* stack, VectorPtr reset);

bool validate(void) {
  // Elevate the watchdog kicker while a cert check is running
  NVIC_SetPriority(TIM14_IRQn, 0);

  // If our hashes do not match, cert is bunk
  bool valid = verify_cert(&APP->signedStart, COZMO_APPLICATION_SIZE - COZMO_APPLICATION_HEADER, APP->certificate, sizeof(APP->certificate));

  NVIC_SetPriority(TIM14_IRQn, 3);

  return valid;
}

static bool boot_test(void) {
  // Failure count reached max
  if (APP->faultCounter[MAX_FAULT_COUNT - 1] != FAULT_NONE) {
    return false;
  }

  // Evil flag not set
  if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
    return false;
  }

  return validate();
}

static const uint16_t MAIN_EXEC_PRESCALE = 4; // Timer prescale
static const uint16_t MAIN_EXEC_PERIOD = SYSTEM_CLOCK / MAIN_EXEC_PRESCALE / 200;        // 200hz (5ms period)

void timer_init(void) {
  // Start our cheese watchdog
  #ifndef DISABLE_WDOG
  WWDG->SR = WWDG_SR_EWIF | 0x7F;
  WWDG->CR = 0xFF;

  // Start the watchdog up
  IWDG->KR = 0xCCCC;
  IWDG->KR = 0x5555;
  IWDG->PR = 0;
  IWDG->RLR = WATCHDOG_LIMIT;
  while (IWDG->SR) ;
  IWDG->WINR = WATCHDOG_WINDOW;
  #endif

  TIM14->PSC = MAIN_EXEC_PRESCALE - 1;
  TIM14->ARR = MAIN_EXEC_PERIOD - 1;
  TIM14->DIER = TIM_DIER_UIE;
  TIM14->CR1 = TIM_CR1_CEN;

  NVIC_EnableIRQ(TIM14_IRQn);
  NVIC_EnableIRQ(WWDG_IRQn);
}

extern "C" void SoftReset(void) {
  NVIC_SystemReset();
}

extern "C" void TIM14_IRQHandler(void) {
  #ifndef DISABLE_WDOG
  IWDG->KR = 0xAAAA;
  WWDG->CR = 0xFF;
  #endif
  TIM14->SR = 0;
}

extern "C" void WWDG_IRQHandler(void) {
  Flash::writeFaultReason(FAULT_WATCHDOG);
}

int main(void) {
  Power::enableClocking();
  Power::init();
  Analog::init();
  Contacts::init();

  timer_init();

  // TODO: WAIT FOR BATTERY VOLTAGE TO GET ABOVE A CERTAIN LEVEL
  // TODO: WAIT FOR VEXT TO GET ABOVE A CERTAIN LEVEL
  
  // Turn power on to the board
  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);

  // If fingerprint is invalid, cert is invalid, or reset counter is zero
  // 1) Wipe flash
  // 2) Start recovery
  if (!boot_test()) {
    Flash::eraseApplication();
  }

  Comms::run();

  // This flag is only set when DFU has validated the image
  if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
    NVIC_SystemReset();
  }

  Analog::stop();
  Power::stop();

  StartApplication(APP->stackStart, APP->resetVector);
}
