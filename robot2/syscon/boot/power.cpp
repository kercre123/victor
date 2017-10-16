#include "common.h"
#include "hardware.h"

#include "power.h"
#include "analog.h"

static const uint16_t MINIMUM_BATTERY = 890; // ~3.6v

void Power::init(void) {
  nCHG_EN::reset();
  nCHG_EN::mode(MODE_OUTPUT);

  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);

  // Make sure battery is partially charged, and that the robot is on a charger
  // NOTE: Only one interrupt is enabled here, and it's the 200hz main timing loop
  // this lowers power consumption and interrupts fire regularly

  do {
    // Wait for a bit with the power on
    POWER_EN::pull(PULL_UP);
    for (int i = 0; i < 10; i++) __asm("wfi");

    // Break out once we have hit a decent charge level
    if (Analog::values[ADC_VBAT] >= MINIMUM_BATTERY) break ;

    // Charge for ~15s bursts
    POWER_EN::pull(PULL_DOWN);
    for (int i = 0; i < 5000; i++) __asm("wfi");
  } while(true);
}

void Power::enableClocking(void) {
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
