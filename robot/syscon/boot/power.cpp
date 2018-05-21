#include "common.h"
#include "hardware.h"

#include "power.h"
#include "analog.h"

void Power::init(void) {
  // Enable clocking on perfs
  RCC->AHBENR |= 0
              | RCC_AHBENR_CRCEN
              | RCC_AHBENR_DMAEN
              | RCC_AHBENR_GPIOAEN
              | RCC_AHBENR_GPIOBEN
              | RCC_AHBENR_GPIOCEN
              | RCC_AHBENR_GPIOFEN
              ;

  RCC->APB1ENR |= 0
               | RCC_APB1ENR_TIM14EN
               ;
  RCC->APB2ENR |= 0
               | RCC_APB2ENR_USART1EN
               | RCC_APB2ENR_SYSCFGEN
               | RCC_APB2ENR_ADC1EN
               ;
}

static inline void disableHead(void) {
  //MAIN_EN::mode(MODE_OUTPUT);
  //MAIN_EN::reset();
}

static inline void enableHead(void) {
  //MAIN_EN::mode(MODE_OUTPUT);
  //MAIN_EN::set();
}

void Power::setMode(PowerMode set) {
  switch (set) {
    case POWER_STOP:
      disableHead();
      Analog::setPower(false);
      break ;
    default:
      enableHead();
      Analog::setPower(true);
      break ;
  }
}
