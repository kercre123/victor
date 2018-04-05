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

  Power::setMode(POWER_ACTIVE);

  // Set N pins on motors low for power up
  LN1::reset();
  LN2::reset();
  HN1::reset();
  HN2::reset();
  RTN1::reset();
  RTN2::reset();
  LTN1::reset();
  LTN2::reset();

  LN1::mode(MODE_OUTPUT);
  LN2::mode(MODE_OUTPUT);
  HN1::mode(MODE_OUTPUT);
  HN2::mode(MODE_OUTPUT);
  RTN1::mode(MODE_OUTPUT);
  RTN2::mode(MODE_OUTPUT);
  LTN1::mode(MODE_OUTPUT);
  LTN2::mode(MODE_OUTPUT);
}


void Power::setMode(PowerMode set) {
  switch (set) {
    case POWER_STOP:
      POWER_EN::pull(PULL_NONE);
      POWER_EN::reset();
      POWER_EN::mode(MODE_OUTPUT);
      break ;
    default:
      POWER_EN::pull(PULL_UP);
      POWER_EN::mode(MODE_INPUT);
      break ;
  }
}

void Power::disableHead(void) {
  MAIN_EN::mode(MODE_OUTPUT);
  MAIN_EN::reset();
}

void Power::enableHead(void) {
  MAIN_EN::mode(MODE_OUTPUT);
  MAIN_EN::set();
}
