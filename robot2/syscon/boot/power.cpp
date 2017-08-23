#include "common.h"
#include "hardware.h"

#include "power.h"
#include "analog.h"

extern "C" void SoftReset(const uint32_t reset);

static const uint32_t APB1_CLOCKS = 0
              | RCC_APB1ENR_TIM14EN
              ;

static const uint32_t APB2_CLOCKS = 0
              | RCC_APB2ENR_USART1EN
              | RCC_APB2ENR_SYSCFGEN
              | RCC_APB2ENR_ADC1EN
              ;

static volatile bool ejectSystem = false;
static const uint16_t MINIMUM_BATTERY = 2310; // ~3.3v

void Power::init(void) {
  nCHG_EN::reset();
  nCHG_EN::mode(MODE_OUTPUT); 

  // Make sure battery is partially charged, and that the robot is on a charger
  while (Analog::values[ADC_VBAT] < MINIMUM_BATTERY) __asm("wfi");

  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);
}

void Power::stop(void) {
  // Turn off charger 
  nCHG_EN::set();

  POWER_EN::mode(MODE_OUTPUT);
  POWER_EN::reset();
}

void Power::enableClocking(void) {
  RCC->AHBENR |= 0
              | RCC_AHBENR_CRCEN
              | RCC_AHBENR_DMAEN
              | RCC_AHBENR_GPIOAEN
              | RCC_AHBENR_GPIOBEN
              | RCC_AHBENR_GPIOCEN
              | RCC_AHBENR_GPIOFEN;

  RCC->APB1ENR = APB1_CLOCKS;
  RCC->APB2ENR = APB2_CLOCKS;
}
