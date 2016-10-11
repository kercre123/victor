#include <stdint.h>

#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"

#include "timer.h"
#include "lights.h"
#include "hal/hardware.h"

typedef uint32_t u32;

static const int MAX_CONTRAST = 0xFFFF;  // must be at least 0x102

static const uint8_t anode = PIN_LED2;
static const uint8_t cathodes[] = { PIN_LED4, PIN_LED3 };

static int channel = 0; // Don't light immediately, because sync causes gross flicker

void Lights::init(void) {
  // Power up our timer
  NRF_TIMER0->POWER = 1;

  NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->PRESCALER = 0;

  NRF_TIMER0->CC[1] = MAX_CONTRAST;  // Maximum bright time

  NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Msk;

  // Configure GPIOTE
  NRF_GPIOTE->POWER = 1;
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
  
  nrf_gpiote_task_disable(0);
  
  // Configure PPI channels
  NRF_PPI->CH[ 9].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
  NRF_PPI->CH[ 9].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
  NRF_PPI->CH[10].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[1];
  NRF_PPI->CH[10].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
  
  NRF_PPI->CHENSET = PPI_CHEN_CH10_Msk | PPI_CHEN_CH9_Msk;
}

void Lights::stop(void) {
  // Disable our timer
  NRF_TIMER0->TASKS_STOP = 1;
  //NRF_TIMER0->POWER = 0;  
  
  // Tear down the PPI configuration
  NRF_PPI->CHENCLR = PPI_CHEN_CH10_Msk | PPI_CHEN_CH9_Msk;

  NRF_PPI->CH[ 9].EEP = 0;
  NRF_PPI->CH[ 9].TEP = 0;
  NRF_PPI->CH[10].EEP = 0;
  NRF_PPI->CH[10].TEP = 0;

  // Disable our GPIOTE tasks
  NRF_GPIOTE->CONFIG[0] = 0;

  //NRF_GPIOTE->POWER = 0;

  // Disable the LED pins
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

void Lights::setChannel(int ch) {
  channel = ch;
}

static void setLight(int colors, uint8_t brightness) {
  // Clear and stop the timer
  NRF_TIMER0->TASKS_STOP = 1;
  NRF_TIMER0->TASKS_CLEAR = 1;

  // Disable GPIOTE task
  nrf_gpiote_task_disable(0);
  
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);

  // Setup our output LED
  nrf_gpio_pin_clear(anode);
  nrf_gpio_cfg_output(anode);
  
  if (colors & 1) {
    nrf_gpio_pin_clear(cathodes[0]);
    nrf_gpio_cfg_output(cathodes[0]);
  }

  if (colors & 2) {
    nrf_gpio_pin_clear(cathodes[1]);
    nrf_gpio_cfg_output(cathodes[1]);
  }

  // Reconfigure which pin 
  nrf_gpiote_task_configure(0, anode, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
  nrf_gpiote_task_enable(0);

  NRF_TIMER0->CC[0] = MAX_CONTRAST - (brightness * brightness) - 1;
  
  NRF_TIMER0->TASKS_START = 1;
}

void Lights::update() {
  static const int period = (int)(10 * COUNT_PER_MS);
  static int next = GetCounter() + period;
  int current = GetCounter();
  
  if (current < next) return ;
  next = current + period;
  
  int intensity = GetCounter() >> 17;

  uint8_t brightness = 0x18 + ((++intensity & 0x40 ? intensity : ~intensity) & 0x3F);

  setLight(channel, brightness);
}
