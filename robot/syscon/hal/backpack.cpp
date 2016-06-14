#include <string.h>

#include "backpack.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "radio.h"

#include "rtos.h"
#include "hardware.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/spineData.h"

using namespace Anki::Cozmo;

extern GlobalDataToBody g_dataToBody;

// Define charlie wiring here:
static const charliePlex_s RGBLightPins[] =
{
  { PIN_LED1, PIN_LED2 },
  { PIN_LED1, PIN_LED4 },
  { PIN_LED2, PIN_LED1 },
  { PIN_LED2, PIN_LED3 },
  { PIN_LED2, PIN_LED4 },
  { PIN_LED3, PIN_LED1 },
  { PIN_LED3, PIN_LED2 },
  { PIN_LED3, PIN_LED4 },
  { PIN_LED4, PIN_LED1 },
  { PIN_LED4, PIN_LED3 },
  { PIN_LED4, PIN_LED2 },
  { PIN_LED4, PIN_LED4 }
};

// Charlieplexing magic constants
static const int numLights = 12;
static const int TIMER_GRAIN = 6;
static const int TIMER_THRESH = 0x10;

static int active_channel = 0;
static const charliePlex_s* currentChannel;
static uint8_t drive_value[numLights];
static uint8_t drive_error[numLights];

// Start all pins as input
void Backpack::init()
{
  currentChannel = &RGBLightPins[0];
  memset(drive_value, 0xFF, sizeof(drive_value));

  static const LightState startUpLight = {
    0x03E0,
    0x0180,
    0x14,0x06,0x14,0x06
  };

  Lights::update(lightController.backpack[2], &startUpLight);
}

static inline void lights_out(void) {
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(currentChannel->cathode, NRF_GPIO_PIN_NOPULL);
}

static void next_channel(void) {
  NRF_TIMER0->TASKS_CAPTURE[3] = 1;

  for(;;) {
    if (++active_channel > numLights) {
      active_channel = 0;
    }

    currentChannel = &RGBLightPins[active_channel];
    int delta = (drive_value[active_channel] << TIMER_GRAIN) - drive_error[active_channel] - TIMER_THRESH;
    
    if (delta > 0) {
      // Turn on our light
      nrf_gpio_pin_set(currentChannel->anode);
      nrf_gpio_cfg_output(currentChannel->anode);
      nrf_gpio_pin_clear(currentChannel->cathode);
      nrf_gpio_cfg_output(currentChannel->cathode);

      NRF_TIMER0->CC[0] = NRF_TIMER0->CC[3] + delta;

      return ;
    } else {
      drive_error[active_channel] = 0;
    }
  }
}

static void setup_timer() {
  NRF_TIMER0->TASKS_STOP = 1;

  NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER0->TASKS_CLEAR = 1;
 
  NRF_TIMER0->PRESCALER = 0;
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
  
  NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);

  next_channel();

  NVIC_EnableIRQ(TIMER0_IRQn);
  NVIC_SetPriority(TIMER0_IRQn, LED_PRIORITY);

  NRF_TIMER0->TASKS_START = 1;
}

void Backpack::lightMode(LightDriverMode mode) {
  switch (mode) {
    case RTC_LEDS:
      NRF_TIMER0->TASKS_STOP = 1;
      NVIC_DisableIRQ(TIMER0_IRQn);
      break ;
    case TIMER_LEDS:
      setup_timer();
      break ;
  }
}

void Backpack::update(void) {
  lights_out();
  
  if (++active_channel > numLights) {
    active_channel = 0;
  }
  currentChannel = &RGBLightPins[active_channel];

  static uint8_t pwm[numLights];

  // Minimum display value
  if (drive_value[active_channel] < 0x18) {
    return ;
  }
  
  // Calculate PDM value
  int overflow = pwm[active_channel] + (int)drive_value[active_channel];
  pwm[active_channel] = overflow;

  if (overflow > 0xFF) {
    nrf_gpio_pin_set(currentChannel->anode);
    nrf_gpio_cfg_output(currentChannel->anode);
    nrf_gpio_pin_clear(currentChannel->cathode);
    nrf_gpio_cfg_output(currentChannel->cathode);
  }
}

void Backpack::manage() { 
  // 8-bit pseudo log scale.  Gives us full bright (64 level)
  static const uint8_t AdjustTable[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x0b,
    0x0d, 0x10, 0x13, 0x17, 0x1b, 0x20, 0x26, 0x2c,
    0x34, 0x3c, 0x45, 0x4f, 0x59, 0x64, 0x70, 0x7c,
    0x88, 0x94, 0x9f, 0xaa, 0xb4, 0xbe, 0xc6, 0xce,
    0xd5, 0xdb, 0xe1, 0xe5, 0xe9, 0xed, 0xef, 0xf2,
    0xf4, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc,
    0xfc, 0xfc, 0xfd, 0xfd, 0xfd, 0xfe, 0xfe, 0xff
  };

  // Channel 1 is unused by the light mapping
  static const int8_t index[] = { 
    0, -1, -1,
    5,  6,  7,
    2,  3,  4,
    8,  9, 10,
    1, -1, -1,
  };

  static const int gamma[] = { 
    0x100, 0x000, 0x000,
    0x0C0, 0x090, 0x100,
    0x0C0, 0x090, 0x100,
    0x0C0, 0x090, 0x100,
    0x100, 0x000, 0x000,
  };
    
  int idx = 0;
  for (int g = 0; g < NUM_BACKPACK_LEDS; g++) {
    uint8_t* levels = lightController.backpack[g].values;

    for (int i = 0; i < 3; i++, idx++) {
      // Disabled channel
      if (index[idx] < 0) continue;
      
      drive_value[index[idx]] = AdjustTable[(gamma[idx] * levels[i]) >> 10];
    }
  }
}

void Backpack::setLights(const LightState* update) {
  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
    Lights::update(lightController.backpack[i], &update[i]);
  }
}

extern "C" void TIMER0_IRQHandler(void) { 
  if (!NRF_TIMER0->EVENTS_COMPARE[0]) {
    return ;
  }
  
  NRF_TIMER0->EVENTS_COMPARE[0] = 0;
  NRF_TIMER0->TASKS_CAPTURE[3] = 1;
  
  lights_out();
  
  drive_error[active_channel] = NRF_TIMER0->CC[3] - drive_value[active_channel];

  next_channel();
}
