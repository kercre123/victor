#include <string.h>

#include "backpack.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "cubes.h"

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
static const int numLights = sizeof(RGBLightPins) / sizeof(RGBLightPins[0]);
static const int TIMER_GRAIN = 6;
static const int TIMER_THRESH = 0x10;

static int active_channel = 0;
static const charliePlex_s* currentChannel = &RGBLightPins[0];
static uint8_t drive_value[numLights];
static uint8_t drive_error[numLights];

// Start all pins as input
void Backpack::init()
{
  // This prevents timers from deadlocking system
  drive_value[numLights - 1] = 0xFF;

  static const LightState startUpLight = {
    0x03E0,
    0x0180,
    0x14,0x06,0x14,0x06
  };

  Lights::update(lightController.backpack[2], &startUpLight);
}

void Backpack::flash()
{
  for (int l = 0; l < 2; l++) {
    for (int i = 0; i < numLights; i++) {

      uint8_t ann = RGBLightPins[i].anode;
      uint8_t cath = RGBLightPins[i].cathode;

      nrf_gpio_pin_set(ann);
      nrf_gpio_cfg_output(ann);
      nrf_gpio_pin_clear(cath);
      nrf_gpio_cfg_output(cath);
      
      MicroWait(10000);
    }
  }
}

static inline void lights_out(void) {
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(currentChannel->cathode, NRF_GPIO_PIN_NOPULL);
}

static void next_channel(void) {
  NRF_TIMER0->TASKS_CAPTURE[3] = 1;

  for(;;) {
    if (++active_channel >= numLights) {
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
  
  if (++active_channel >= numLights) {
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
  struct BackpackLightAdjust {
    uint8_t pos;
    uint8_t index;
    uint8_t gamma;
  };
  
  static const BackpackLightAdjust setting[] = {
    { 0, 0, 0x10 }, // 0
    { 4, 0, 0x10 }, // 1
    { 2, 0, 0x0C }, // 2
    { 2, 1, 0x09 }, // 3
    { 2, 2, 0x10 }, // 4
    { 1, 0, 0x0C }, // 5
    { 1, 1, 0x09 }, // 6
    { 1, 2, 0x10 }, // 7
    { 3, 0, 0x0C }, // 8
    { 3, 1, 0x09 }, // 9
    { 3, 2, 0x10 }, // 10
  };
  
  static const int lightCount = sizeof(setting) / sizeof(setting[0]);

  for (int i = 0; i < lightCount; i++) {
    const BackpackLightAdjust& light = setting[i];
    uint8_t* rgbi = (uint8_t*) &lightController.backpack[light.pos].values;
    uint32_t drive = rgbi[light.index] * light.gamma;
    drive_value[i] = (drive * drive) >> 16;
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
