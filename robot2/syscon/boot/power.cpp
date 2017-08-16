#include "common.h"
#include "hardware.h"

#include "power.h"

#include "contacts.h"

extern "C" void SoftReset(const uint32_t reset);

static const uint32_t APB1_CLOCKS = 0
              | RCC_APB1ENR_USART2EN
              | RCC_APB1ENR_TIM3EN
              | RCC_APB1ENR_TIM6EN
              | RCC_APB1ENR_TIM14EN
              ;

static const uint32_t APB2_CLOCKS = 0
              | RCC_APB2ENR_USART1EN
              | RCC_APB2ENR_TIM1EN
              | RCC_APB2ENR_TIM15EN
              | RCC_APB2ENR_TIM16EN
              | RCC_APB2ENR_TIM17EN
              | RCC_APB2ENR_SYSCFGEN
              | RCC_APB2ENR_ADC1EN
              ;

static volatile bool ejectSystem = false;

void Power::init(void) {
  nCHG_EN::reset();
  nCHG_EN::mode(MODE_OUTPUT);
}

void Power::stop(void) {
  nCHG_EN::set();
  
  POWER_EN::mode(MODE_OUTPUT);
  POWER_EN::reset();
}

void Power::enableClocking(void) {
  RCC->APB1ENR |= APB1_CLOCKS;
  RCC->APB2ENR |= APB2_CLOCKS;
}

