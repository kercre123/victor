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

// Define charlie wiring here:
struct BackpackLight {
  uint8_t controller_pos;
  uint8_t controller_index;
  uint8_t anode;
  uint8_t cathode;
  uint8_t gamma;
};

static const BackpackLight setting[] = {
  { 0, 0, PIN_LED1, PIN_LED2, 0x10 }, // 0
  { 4, 0, PIN_LED1, PIN_LED4, 0x10 }, // 1
  
  { 1, 0, PIN_LED3, PIN_LED1, 0x0D }, // 5
  { 1, 1, PIN_LED3, PIN_LED2, 0x0B }, // 6
  { 1, 2, PIN_LED3, PIN_LED4, 0x10 }, // 7

  { 2, 0, PIN_LED2, PIN_LED1, 0x0D }, // 2
  { 2, 1, PIN_LED2, PIN_LED3, 0x0B }, // 3
  { 2, 2, PIN_LED2, PIN_LED4, 0x10 }, // 4

  { 3, 0, PIN_LED4, PIN_LED1, 0x0D }, // 8
  { 3, 1, PIN_LED4, PIN_LED3, 0x0B }, // 9
  { 3, 2, PIN_LED4, PIN_LED2, 0x10 }, // 10
};

static const int LIGHT_COUNT = sizeof(setting) / sizeof(setting[0]);

static uint32_t TIMER_MINIMUM = 0xC0;
static uint32_t TIMER_DIVIDE = 9; // This must be AT LEAST 8
static uint32_t DARK_TIME = 0xFFFF00 >> TIMER_DIVIDE;

static LightState lightState[BACKPACK_LAYERS][BACKPACK_LIGHTS];
static BackpackLayer currentLayer = BPL_IMPULSE;
static CurrentChargeState chargeState = CHARGE_OFF_CHARGER;
static bool isBatteryLow = false;

extern "C" void TIMER1_IRQHandler(void);

// Charlieplexing magic constants
static uint32_t led_value[LIGHT_COUNT];

// Start all pins as input
void Backpack::init()
{
  setLayer(BPL_IMPULSE);
  setLights(BPL_IMPULSE, BackpackLights::disconnected);
  
  // Clear out backpack leds
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

void Backpack::manage() {
  for (int i = 0; i < LIGHT_COUNT; i++) {
    const BackpackLight& light = setting[i];
    uint8_t* rgbi = (uint8_t*) &lightController.backpack[light.controller_pos].values;
    uint32_t drive = light.gamma * (uint32_t)rgbi[light.controller_index];

    // We shift by 16 to adjust for the gain from gamma (which is a 24:8 fixed)
    led_value[i] = (drive * drive) >> TIMER_DIVIDE;
  }
}

static void setImpulsePattern(void) {
  using namespace Backpack;

  switch (chargeState) {
    case CHARGE_OFF_CHARGER:
      setLights(BPL_IMPULSE, isBatteryLow ? BackpackLights::low_battery : BackpackLights::disconnected);
      break ;

    case CHARGE_CHARGING:
      setLights(BPL_IMPULSE, BackpackLights::charging);
      break ;

    case CHARGE_CHARGED:
      setLights(BPL_IMPULSE, BackpackLights::charged);
      break ;

    case CHARGE_CHARGER_OUT_OF_SPEC:
      setLights(BPL_IMPULSE, BackpackLights::low_battery);
      break ;
  }
}

void Backpack::lightsOff() {
  Backpack::setLights(BPL_IMPULSE, BackpackLights::all_off);
  Backpack::setLayer(BPL_IMPULSE);
}

void Backpack::setLowBattery(bool batteryLow) {
  if (batteryLow == isBatteryLow) {
    return ;
  }
  
  isBatteryLow = batteryLow;
  setImpulsePattern();
}

void Backpack::setChargeState(CurrentChargeState state) {
  if (chargeState == state) {
    return ;
  }
  
  chargeState = state;
  setImpulsePattern();
}

static void updateLights(const LightState* update) {
  using namespace Backpack;

  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
    Lights::update(lightController.backpack[i], &update[i]);
  }
}

void Backpack::setLayer(BackpackLayer layer) {
  if (layer == currentLayer) {
    return ;
  }
  
  currentLayer = layer;
  updateLights(lightState[currentLayer]);
}

void Backpack::setLights(BackpackLayer layer, const LightState* update) {
  memcpy(lightState[layer], update, sizeof(LightState)*BACKPACK_LIGHTS);

  if (currentLayer == layer) {
    updateLights(lightState[layer]);
  }
}

void Backpack::setLightsMiddle(BackpackLayer layer, const LightState* update) {
  // Middle lights are indicies 1,2,3
  memcpy(&lightState[layer][1], update, sizeof(LightState)*3);

  if (currentLayer == layer) {
    updateLights(lightState[layer]);
  }
}

void Backpack::setLightsTurnSignals(BackpackLayer layer, const LightState* update) {
  // Turnsignal lights are indicies 0,4
  memcpy(&lightState[layer][0], &update[0], sizeof(LightState));
  memcpy(&lightState[layer][4], &update[1], sizeof(LightState));
  
  if (currentLayer == layer) {
    updateLights(lightState[layer]);
  }
}

void Backpack::useTimer() {
  NRF_TIMER1->POWER = 1;
  
  NRF_TIMER1->TASKS_STOP = 1;
  NRF_TIMER1->TASKS_CLEAR = 1;
  
  NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
  NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER1->PRESCALER = 0;

  NRF_TIMER1->CC[1] = DARK_TIME;
  
  NRF_TIMER1->INTENCLR = ~0;
  NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk
                       | TIMER_INTENSET_COMPARE1_Msk;

  NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Msk;

  NRF_TIMER1->TASKS_START = 1;

  NVIC_SetPriority(TIMER1_IRQn, LIGHT_PRIORITY);
  NVIC_EnableIRQ(TIMER1_IRQn);
}

void Backpack::detachTimer() {
  // Detach from the timer
  NRF_TIMER1->INTENCLR = ~0;
  NVIC_DisableIRQ(TIMER1_IRQn);
  NRF_TIMER1->TASKS_STOP = 1;

  // Clear out backpack leds
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

extern "C" void TIMER1_IRQHandler(void) { 
  static uint32_t drive_value[LIGHT_COUNT];
  static int active_channel = 0;

  // Turn off lights
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);

  // This just clears out lights
  NRF_TIMER1->EVENTS_COMPARE[0] = 0;

  // Setup next LED
  if (NRF_TIMER1->EVENTS_COMPARE[1]) {
    NRF_TIMER1->EVENTS_COMPARE[1] = 0;

    if (++active_channel >= LIGHT_COUNT) {
      active_channel = 0;

      // Prevent sheering of single LED value
      memcpy(drive_value, led_value, sizeof(drive_value));
    }

    const BackpackLight* currentChannel = &setting[active_channel];

    uint32_t delta = drive_value[active_channel];
    NRF_TIMER1->CC[0] = delta;
    
    if (delta > TIMER_MINIMUM) {
      // Turn on our light
      nrf_gpio_pin_clear(currentChannel->cathode);
      nrf_gpio_pin_set(currentChannel->anode);

      nrf_gpio_cfg_output(currentChannel->cathode);
      nrf_gpio_cfg_output(currentChannel->anode);
    }
  }
}
