#include "vectors.h"
#include "bignum.h"
#include "rsa_pss.h"
#include "flash.h"
#include "comms.h"

#include "stm32f0xx.h"

#include "common.h"
#include "hardware.h"

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

static const uint32_t MAIN_EXEC_TICK  = 1000000; // Microseconds
static const uint16_t MAIN_EXEC_PRESCALE = SYSTEM_CLOCK / MAIN_EXEC_TICK; // Timer prescale
static const uint16_t MAIN_EXEC_PERIOD = MAIN_EXEC_TICK / 200;        // 200hz (5ms period)

void timer_init(void) {
  // Start our cheese watchdog
  WWDG->SR = WWDG_SR_EWIF | 0x7F;
  WWDG->CR = 0xFF;

  // Start the watchdog up
  IWDG->KR = 0xCCCC;
  IWDG->KR = 0x5555;
  IWDG->PR = 0;
  IWDG->RLR = WATCHDOG_LIMIT;
  while (IWDG->SR) ;
  IWDG->WINR = WATCHDOG_WINDOW;

  TIM14->PSC = MAIN_EXEC_PRESCALE - 1;
  TIM14->ARR = MAIN_EXEC_PERIOD - 1;
  TIM14->DIER = TIM_DIER_UIE;
  TIM14->CR1 = TIM_CR1_CEN;

  NVIC_EnableIRQ(TIM14_IRQn);
  NVIC_EnableIRQ(WWDG_IRQn);
}

extern "C" void TIM14_IRQHandler(void) {
  IWDG->KR = 0xAAAA;
  WWDG->CR = 0xFF;
  TIM14->SR = 0;
}

extern "C" void WWDG_IRQHandler(void) {
  Flash::writeFaultReason(FAULT_WATCHDOG);
}

int main(void) {
  // Enable DMA, CRC, GPIO and USART1
  RCC->AHBENR |= 0
              | RCC_APB1ENR_WWDGEN
              | RCC_AHBENR_CRCEN
              | RCC_AHBENR_DMAEN
              | RCC_AHBENR_GPIOAEN
              | RCC_AHBENR_GPIOBEN
              | RCC_AHBENR_GPIOCEN
              | RCC_AHBENR_GPIOFEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  // Start the watchdog up
  timer_init();

  // Turn power on to the board
  POWER_EN::set();
  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);

  // If fingerprint is invalid, cert is invalid, or reset counter is zero
  // 1) Wipe flash
  // 2) Start recovery
  if (!boot_test()) {
    Flash::eraseApplication();
  }

  if (APP->fingerPrint == COZMO_APPLICATION_FINGERPRINT) {
    APP->visitorInit();
  }

  Comms::run();

  // This flag is only set when DFU has validated the image
  if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
    NVIC_SystemReset();
  }

  StartApplication(APP->stackStart, APP->resetVector);
}
