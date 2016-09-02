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
  
  { 2, 0, PIN_LED2, PIN_LED1, 0x0C }, // 2
  { 2, 1, PIN_LED2, PIN_LED3, 0x09 }, // 3
  { 2, 2, PIN_LED2, PIN_LED4, 0x10 }, // 4
  
  { 1, 0, PIN_LED3, PIN_LED1, 0x0C }, // 5
  { 1, 1, PIN_LED3, PIN_LED2, 0x09 }, // 6
  { 1, 2, PIN_LED3, PIN_LED4, 0x10 }, // 7
  
  { 3, 0, PIN_LED4, PIN_LED1, 0x0C }, // 8
  { 3, 1, PIN_LED4, PIN_LED3, 0x09 }, // 9
  { 3, 2, PIN_LED4, PIN_LED2, 0x10 }, // 10
};

static const int LIGHT_COUNT = sizeof(setting) / sizeof(setting[0]);

static const int TIMER_GRAIN = 7;//10;
static const int TIMER_DELTA_MINIMUM = 4 << TIMER_GRAIN;
static const int DARK_TIME = ((LIGHT_COUNT + 1) * 0x100) << TIMER_GRAIN;

static LightState lightState[BACKPACK_LAYERS][BACKPACK_LIGHTS];
static BackpackLayer currentLayer = BPL_IMPULSE;

static void backpack_update(NRF_TIMER_Type* const timer);

// Charlieplexing magic constants
static int led_value[LIGHT_COUNT];

// Start all pins as input
void Backpack::init()
{
  setLayer(BPL_IMPULSE);
  setLights(BPL_IMPULSE, BackpackLights::all_off);
  
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
    uint32_t drive = rgbi[light.controller_index] * light.gamma;

    // We shift by 16 to adjust for the gain from gamma (which is a 24:8 fixed)
    int value = (drive * drive) >> (16 - TIMER_GRAIN);
    led_value[i] = (value > TIMER_DELTA_MINIMUM) ? value : 0;
  }
}

static bool isBatteryLow = false;
static CurrentChargeState chargeState = CHARGE_OFF_CHARGER;

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
  memcpy(lightState[layer], update, sizeof(LightState)*4);

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

void Backpack::useTimer(NRF_TIMER_Type* timer, IRQn_Type interrupt) {
  timer->POWER = 1;
  
  timer->TASKS_STOP = 1;
  timer->TASKS_CLEAR = 1;
  
  timer->MODE = TIMER_MODE_MODE_Timer;
  timer->PRESCALER = 0;
  timer->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
  
  timer->INTENCLR = ~0;
  timer->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

  backpack_update(timer);
  timer->SHORTS = 0;

  timer->TASKS_START = 1;

  NVIC_SetPriority(interrupt, LIGHT_PRIORITY);
  NVIC_EnableIRQ(interrupt);
}

void Backpack::detachTimer(NRF_TIMER_Type* timer, IRQn_Type interrupt) {
  // Detach from the timer
  timer->INTENCLR = ~0;
  NVIC_DisableIRQ(interrupt);
  timer->TASKS_STOP = 1;

  // Clear out backpack leds
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

static void backpack_update(NRF_TIMER_Type* const timer) {
  static int drive_value[LIGHT_COUNT];
  static int active_channel = 0;
  static const BackpackLight* currentChannel = &setting[0];
  static int blackout_time = DARK_TIME;
  int delta;

  timer->EVENTS_COMPARE[0] = 0;

  // Turn off lights
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(currentChannel->cathode, NRF_GPIO_PIN_NOPULL);

  // Skip lights too dim to matter, set to full black
  do {
    // Select next channel
    if (++active_channel >= LIGHT_COUNT) {
      active_channel = -1;

      int dark_time = blackout_time;
      blackout_time = DARK_TIME;
      
      // Prevent sheering of single LED value
      memcpy(drive_value, led_value, sizeof(drive_value));
      
      timer->TASKS_CAPTURE[0] = 1;
      timer->CC[0] += dark_time;
      return ;
    }

    delta = drive_value[active_channel];
  } while (!delta);

  currentChannel = &setting[active_channel];
  blackout_time -= delta;
  
  // Setup next interrupt
  timer->TASKS_CAPTURE[0] = 1;
  timer->CC[0] += delta;

  // Drive cathode
  nrf_gpio_pin_clear(currentChannel->cathode);
  nrf_gpio_cfg_output(currentChannel->cathode);

  // Turn on our anode
  nrf_gpio_pin_set(currentChannel->anode);
  nrf_gpio_cfg_output(currentChannel->anode);
}

extern "C" void TIMER0_IRQHandler(void) { 
  backpack_update(NRF_TIMER0);
}

extern "C" void TIMER1_IRQHandler(void) { 
  backpack_update(NRF_TIMER1);
}

