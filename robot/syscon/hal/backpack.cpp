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

#include "backpackLightData.h"

using namespace Anki::Cozmo;

extern GlobalDataToBody g_dataToBody;

static const int CATHODE_COUNT = 3;
static const int CHANNEL_COUNT = 4;

static const int TIMER_GRAIN = 2;
static const int TIMER_DELTA_MINIMUM = 2;
static const int MAX_DARK = (0x100 >> TIMER_GRAIN) + 16;

static LightState _userLights[BACKPACK_LIGHT_CHANNELS];
static bool lights_locked;

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
static bool active[CATHODE_COUNT] = { false, false, false };
static int total_active = 0;
static int off_time = 0;

// Start all pins as input
void Backpack::init()
{
  setLights(BackpackLights::startup);
  defaultPattern(LIGHTS_RELEASE);
  
  // Prime our counter
  Backpack::update(0);
}

// This is a temporary fix until I can make the comparisons not jam
static void unjam(void) {
  static const uint32_t STALLED_PERIOD = 0x8000;

  for (int i = 0; i < 3; i++) {
    uint32_t count = (NRF_RTC1->CC[i] - NRF_RTC1->COUNTER) << 8;
    if (count < STALLED_PERIOD) return ;
  }
  
  for (int i = 0; i < 3; i++) {
    active[i] = false;
    total_active = 0;
  }

  NRF_RTC1->CC[0] = NRF_RTC1->COUNTER + 0x20;
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
    { 4, 0, 0, 2, 0x10 }, // 1
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

  unjam();
}

void Backpack::blink(void) {
  static const int pins[] = {
    PIN_LED1,
    PIN_LED2,
    PIN_LED3,
    PIN_LED4,
  };
  static const int pin_count = sizeof(pins) / sizeof(pins[0]);
  
  for (int k = 0; k < 4; k++)
  for (int i = 0; i < pin_count; i++) {
    for (int p = 0; p < pin_count; p++) {
      if (i == p) continue ;

      nrf_gpio_pin_set(pins[i]);
      nrf_gpio_cfg_output(pins[i]);
      nrf_gpio_pin_clear(pins[p]);
      nrf_gpio_cfg_output(pins[p]);

      MicroWait(10000);

      nrf_gpio_cfg_input(pins[i], NRF_GPIO_PIN_NOPULL);
      nrf_gpio_cfg_input(pins[p], NRF_GPIO_PIN_NOPULL);
    }
  }
}

static void updateLights(const LightState* update) {
  using namespace Backpack;

  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
    Lights::update(lightController.backpack[i], &update[i]);
  }
}

void Backpack::defaultPattern(DefaultBackpackPattern pattern) {
  switch (pattern) {
    case LIGHTS_RELEASE:
      lights_locked = false;
      updateLights(_userLights);
      break ;
    case LIGHTS_CHARGING:
      lights_locked = true;
      updateLights(BackpackLights::charging);
      break ;
    case LIGHTS_CHARGED:
      lights_locked = true;
      updateLights(BackpackLights::charged);
      break ;
    case LIGHTS_CHARGER_OOS:
      lights_locked = true;
      updateLights(BackpackLights::charger_oos);
      break ;
    case LIGHTS_SLEEPING:
      lights_locked = false;
      updateLights(BackpackLights::off);
      break ;
  };
}

void Backpack::setLights(const LightState* update) {
  memcpy(_userLights, update, sizeof(_userLights));
  
  if (lights_locked) {
    return ;
  }
  
  updateLights(_userLights);
}

void Backpack::update(int compare) { 
  // Turn off channel that is currently active
  if (active[compare]) {
    nrf_gpio_cfg_input(currentChannel->cathodes[compare], NRF_GPIO_PIN_NOPULL);
    active[compare] = false;

    // We want to go dark
    if (--total_active == 0) {
      nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
      NRF_RTC1->CC[0] = off_time;
    }
    
    // We turned off an LED this cycle, wait a few more ticks before moving on
    return ;
  }

  // Select next channel
  if (++active_channel >= CHANNEL_COUNT) {
    active_channel = 0;
  }

  currentChannel = &PinSet[active_channel];
  off_time = NRF_RTC1->COUNTER + MAX_DARK;
  NRF_RTC1->CC[0] = off_time;

  // Turn on our anode
  nrf_gpio_pin_set(currentChannel->anode);
  nrf_gpio_cfg_output(currentChannel->anode);

  // Light LEDs for required amount of time
  for (int cath = 0; cath < CATHODE_COUNT; cath++) {
    int delta = drive_value[active_channel][cath] >> TIMER_GRAIN;
    
    if (delta < TIMER_DELTA_MINIMUM) {
      nrf_gpio_cfg_input(currentChannel->cathodes[cath], NRF_GPIO_PIN_NOPULL);
      continue ;
    }

    NRF_RTC1->CC[cath] = NRF_RTC1->COUNTER + delta;

    nrf_gpio_pin_clear(currentChannel->cathodes[cath]);
    nrf_gpio_cfg_output(currentChannel->cathodes[cath]);
    
    active[cath] = true;
    total_active++;
  }
}
