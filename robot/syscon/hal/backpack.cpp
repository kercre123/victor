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

extern const unsigned int COZMO_VERSION_COMMIT;

extern GlobalDataToBody g_dataToBody;

static const int CATHODE_COUNT = 3;
static const int CHANNEL_COUNT = 4;

static const int TIMER_GRAIN = 2;
static const int TIMER_DELTA_MINIMUM = 2;
static const int DARK_TIME_OFFSET = 4;
static const int LIGHT_TIME = (0x100 >> TIMER_GRAIN) + DARK_TIME_OFFSET;

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathodes[CATHODE_COUNT];
};

// Define charlie wiring here:

static const charliePlex_s PinSet[CHANNEL_COUNT] =
{
  { PIN_LED1, { PIN_LED2, PIN_LED3, PIN_LED4 } },
  { PIN_LED2, { PIN_LED1, PIN_LED3, PIN_LED4 } },
  { PIN_LED3, { PIN_LED4, PIN_LED1, PIN_LED2 } },
  { PIN_LED4, { PIN_LED1, PIN_LED3, PIN_LED2 } },
};

// Charlieplexing magic constants
static int active_channel = 0;
static const charliePlex_s* currentChannel = &PinSet[0];
static uint8_t drive_value[CHANNEL_COUNT][CATHODE_COUNT];

// Start all pins as input
void Backpack::init()
{
  // This prevents timers from deadlocking system
  static const LightState lights[] = {
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { COZMO_VERSION_COMMIT & 0xFFFF, 0x0000, 34, 67, 17, 17 },
    { 0x7FFF, 0x0000, 34, 67, 17, 17 },
    { COZMO_VERSION_COMMIT >> 16, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 }
  };

  setLights(lights);

  NRF_RTC1->CC[TIMER_CC_LIGHTS_OFF] = NRF_RTC1->COUNTER + 0x100;
}

void Backpack::manage() {
  struct BackpackLightAdjust {
    uint8_t controller_pos;
    uint8_t controller_index;
    uint8_t channel;
    uint8_t cathode;
    uint8_t gamma;
  };
  
  static const BackpackLightAdjust setting[] = {
    { 0, 0, 0, 0, 0x10 }, // 0
    { 4, 0, 0, 1, 0x10 }, // 1
    { 2, 0, 1, 0, 0x0C }, // 2
    { 2, 1, 1, 1, 0x09 }, // 3
    { 2, 2, 1, 2, 0x10 }, // 4
    { 1, 0, 2, 1, 0x0C }, // 5
    { 1, 1, 2, 2, 0x09 }, // 6
    { 1, 2, 2, 0, 0x10 }, // 7
    { 3, 0, 3, 0, 0x0C }, // 8
    { 3, 1, 3, 1, 0x09 }, // 9
    { 3, 2, 3, 2, 0x10 }, // 10
  };

  static const int lightCount = sizeof(setting) / sizeof(setting[0]);

  for (int i = 0; i < lightCount; i++) {
    const BackpackLightAdjust& light = setting[i];
    uint8_t* rgbi = (uint8_t*) &lightController.backpack[light.controller_pos].values;
    uint32_t drive = rgbi[light.controller_index] * light.gamma;
    drive_value[light.channel][light.cathode] = (drive * drive) >> 16;
  }
}

void Backpack::setLights(const LightState* update) {
  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
    Lights::update(lightController.backpack[i], &update[i]);
  }
}

static int counter;

static void manageLights(void) {
  int minimum = LIGHT_TIME - counter;
  bool active = false;

  for (int cath = 0; cath < CATHODE_COUNT; cath++) {
    int delta = (drive_value[active_channel][cath] >> TIMER_GRAIN) - counter;
  
    // This light is now off
    if (delta < TIMER_DELTA_MINIMUM) {
      nrf_gpio_cfg_input(currentChannel->cathodes[cath], NRF_GPIO_PIN_NOPULL);
      continue ;
    }

    nrf_gpio_pin_clear(currentChannel->cathodes[cath]);
    nrf_gpio_cfg_output(currentChannel->cathodes[cath]);

    active = true;
    if (delta < minimum) {
      minimum = delta;
    }
  }

  if (active) { 
    NRF_RTC1->CC[TIMER_CC_LIGHTS_VALUE] = NRF_RTC1->COUNTER + minimum;
  }
}

void Backpack::lightsOff(void) {
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);

  if (++active_channel >= CHANNEL_COUNT) {
    active_channel = 0;
  }
  
  currentChannel = &PinSet[active_channel];
  counter = 0;
  
  manageLights();

  NRF_RTC1->CC[TIMER_CC_LIGHTS_OFF] = NRF_RTC1->COUNTER + LIGHT_TIME;

  nrf_gpio_pin_set(currentChannel->anode);
  nrf_gpio_cfg_output(currentChannel->anode);
}

void Backpack::lightsValue(void) {
  manageLights();
}
