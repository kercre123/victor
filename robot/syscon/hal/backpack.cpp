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

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathode;
};

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
  { PIN_LED4, PIN_LED2 }
};

// Charlieplexing magic constants
static const int numLights = sizeof(RGBLightPins) / sizeof(RGBLightPins[0]);

static int active_channel = 0;
static const charliePlex_s* currentChannel = &RGBLightPins[0];
static uint8_t drive_value[numLights];
static int drive_error[numLights];

// Helper function
static inline void light_off(void) {
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(currentChannel->cathode, NRF_GPIO_PIN_NOPULL);
}

static inline void light_on(void) {
  nrf_gpio_pin_set(currentChannel->anode);
  nrf_gpio_cfg_output(currentChannel->anode);
  nrf_gpio_pin_clear(currentChannel->cathode);
  nrf_gpio_cfg_output(currentChannel->cathode);
}

// Start all pins as input
void Backpack::init()
{
  // This prevents timers from deadlocking system
  static const LightState lights[] = {
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x7FFF, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 }
  };

  setLights(lights);

  NRF_RTC1->CC[1] = NRF_RTC1->COUNTER + 0x100;
}

void Backpack::flash()
{
  for (int l = 0; l < 2; l++) {
    for (int i = 0; i < numLights; i++) {
      currentChannel = &RGBLightPins[i];
      light_on();
      MicroWait(10000);
    }
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

// RTC powered lights
static const int TIMER_GRAIN = 3;
static const int TIMER_DELTA_MINIMUM = 0x10;

void Backpack::update(void) { 
  // Clear out our interupt
  if (active_channel >= 0) {
    drive_error[active_channel] = NRF_RTC1->COUNTER - NRF_RTC1->CC[1];
    light_off();
  }

  // Locate next channel with appropriate brightness
  int delta;

  do {
    // Did we reach the end of our time together
    if (++active_channel >= numLights) {
      NRF_RTC1->CC[1] = (NRF_RTC1->COUNTER + 0x400) & ~0x3FF;
      active_channel = -1;
      return ;
    }
    
    delta = (drive_value[active_channel] >> TIMER_GRAIN) - drive_error[active_channel];
    drive_error[active_channel] = 0;
  } while (delta < TIMER_DELTA_MINIMUM);

  // Light that mo-fo up
  currentChannel = &RGBLightPins[active_channel];
  NRF_RTC1->CC[1] = NRF_RTC1->COUNTER + delta;

  light_on();
}
