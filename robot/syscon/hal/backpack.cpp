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

#include "messages.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

//#define DISABLE_LIGHTS

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
  { 1, 0, PIN_LED1, PIN_LED4, 0x10 }, // 1
  { 3, 0, PIN_LED2, PIN_LED1, 0x0D }, // 2
  { 3, 1, PIN_LED2, PIN_LED3, 0x0B }, // 3
  { 3, 2, PIN_LED2, PIN_LED4, 0x10 }, // 4
  { 2, 0, PIN_LED3, PIN_LED1, 0x0D }, // 5
  { 2, 1, PIN_LED3, PIN_LED2, 0x0B }, // 6
  { 2, 2, PIN_LED3, PIN_LED4, 0x10 }, // 7
  { 4, 0, PIN_LED4, PIN_LED1, 0x0D }, // 8
  { 4, 1, PIN_LED4, PIN_LED3, 0x0B }, // 9
  { 4, 2, PIN_LED4, PIN_LED2, 0x10 }, // 10
  { 0, 0, PIN_LED1, PIN_LED1,    0 }, // 10
};

static const int LIGHT_COUNT = sizeof(setting) / sizeof(setting[0]) - 1;

static uint32_t TIMER_MINIMUM = 0xC0;
static uint32_t TIMER_DIVIDE  = 9;
static uint32_t TIMER_SKIPS   = 4;

static LightState lightState[BACKPACK_LAYERS][BACKPACK_LIGHTS];
static BackpackLayer currentLayer = BPL_IMPULSE;
static CurrentChargeState chargeState = CHARGE_OFF_CHARGER;
static uint32_t drive_value[LIGHT_COUNT];

static bool isBatteryLow = false;
static bool override = false;
static bool button_pressed = false;
static bool lights_enabled = false;

extern "C" void TIMER1_IRQHandler(void);

// Start all pins as input
void Backpack::init()
{
  lightsOff();
  setLightsMiddle(BPL_IMPULSE, BackpackLights::disconnected);
  
  // Clear out backpack leds
  #ifndef DISABLE_LIGHTS
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
  #else
  nrf_gpio_cfg_output(PIN_LED1);
  nrf_gpio_cfg_output(PIN_LED2);
  nrf_gpio_cfg_output(PIN_LED3);
  nrf_gpio_cfg_output(PIN_LED4);
  #endif
  
  override = false;
}

void Backpack::testLight(int channel) {
  if (channel > 0) {
    drive_value[channel-1] = 0xFFFF >> TIMER_DIVIDE;
    override = true;
  } else {
    override = false;
  }
}

void Backpack::manage() {
  static bool was_button_pressed = false;

  if (was_button_pressed != button_pressed) {
    RobotInterface::BackpackButton msg;
    msg.depressed = button_pressed;
    RobotInterface::SendMessage(msg);

    was_button_pressed = button_pressed;
  }

  if (override) return ;

  for (int i = 0; i < LIGHT_COUNT; i++) {
    const BackpackLight& light = setting[i];
    uint8_t* rgbi = (uint8_t*) &lightController.backpack[light.controller_pos].values;
    uint32_t drive = light.gamma * (uint32_t)rgbi[light.controller_index];

    // We shift by 16 to adjust for the gain from gamma (which is a 24:8 fixed)
    drive_value[i] = (drive * drive) >> TIMER_DIVIDE;
  }
}

static void setImpulsePattern(void) {
  using namespace Backpack;

  switch (chargeState) {
    case CHARGE_OFF_CHARGER:
      setLightsMiddle(BPL_IMPULSE, isBatteryLow ? BackpackLights::low_battery : BackpackLights::disconnected);
      break ;

    case CHARGE_CHARGING:
      setLightsMiddle(BPL_IMPULSE, BackpackLights::charging);
      break ;

    case CHARGE_CHARGED:
      setLightsMiddle(BPL_IMPULSE, BackpackLights::charged);
      break ;

    case CHARGE_CHARGER_OUT_OF_SPEC:
      setLightsMiddle(BPL_IMPULSE, BackpackLights::low_battery);
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
  if (layer == currentLayer || layer >= BACKPACK_LAYERS) {
    return ;
  }
  
  currentLayer = layer;
  updateLights(lightState[currentLayer]);
}

void Backpack::clearLights(BackpackLayer layer) {
  if (layer >= BACKPACK_LAYERS) return ;

  Backpack::setLights(layer, BackpackLights::all_off);
}

void Backpack::setLights(BackpackLayer layer, const LightState* update) {
  if (layer >= BACKPACK_LAYERS) return ;

  memcpy(lightState[layer], update, sizeof(LightState)*BACKPACK_LIGHTS);

  if (currentLayer == layer) {
    updateLights(lightState[layer]);
  }
}

void Backpack::setLightsMiddle(BackpackLayer layer, const LightState* update) {
  if (layer >= BACKPACK_LAYERS) return ;

  // Middle lights are indicies 2,3,4
  memcpy(&lightState[layer][2], update, sizeof(LightState)*3);

  if (currentLayer == layer) {
    updateLights(lightState[layer]);
  }
}

void Backpack::setLightsTurnSignals(BackpackLayer layer, const LightState* update) {
  if (layer >= BACKPACK_LAYERS) return ;

  // Turnsignal lights are indicies 0,1
  memcpy(&lightState[layer][0], &update[0], sizeof(LightState)*2);
  
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

  NRF_TIMER1->INTENCLR = ~0;
  NRF_TIMER1->SHORTS = 0;

  NRF_TIMER1->TASKS_START = 1;

  NVIC_SetPriority(TIMER1_IRQn, LIGHT_PRIORITY);
  NVIC_EnableIRQ(TIMER1_IRQn);

  lights_enabled = true;
}

void Backpack::detachTimer() {
  // Detach from the timer
  NRF_TIMER1->INTENCLR = ~0;
  NVIC_DisableIRQ(TIMER1_IRQn);
  NRF_TIMER1->TASKS_STOP = 1;

  // Clear out backpack leds
  #ifndef DISABLE_LIGHTS
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
  #endif

  lights_enabled = false;
}

void Backpack::trigger() {
  static int countdown = 0;

  if (--countdown > 0) {
    return ;
  }
  countdown = TIMER_SKIPS;

  #ifndef DISABLE_LIGHTS
  switch (*HW_VERSION) {
  case 0x01050000:
    nrf_gpio_pin_clear(PIN_BUTTON_DRIVE);
    nrf_gpio_cfg_output(PIN_BUTTON_DRIVE);
    MicroWait(100);
    button_pressed = !nrf_gpio_pin_read(PIN_BUTTON_SENSE);
    nrf_gpio_cfg_input(PIN_BUTTON_DRIVE, NRF_GPIO_PIN_NOPULL);

    Battery::hookButton(button_pressed);
  }

  if (lights_enabled) TIMER1_IRQHandler();
  #endif
}

extern "C" void TIMER1_IRQHandler(void) { 
  static const BackpackLight* currentChannel = &setting[0];
  static int active_channel = 0;

    // This just clears out lights
  NRF_TIMER1->EVENTS_COMPARE[0] = 0;

  // Turn off lights
  nrf_gpio_cfg_input(currentChannel->anode, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(currentChannel->cathode, NRF_GPIO_PIN_NOPULL);

  // Setup next LED
  for (;;) {
    currentChannel = &setting[active_channel];
    uint32_t delta = drive_value[active_channel];
    ++active_channel;
    
    if (currentChannel->gamma == 0 ) {
      active_channel = 0;
      NRF_TIMER1->INTENCLR = TIMER_INTENSET_COMPARE0_Msk;
      return ;
    }

    if (delta >= TIMER_MINIMUM) {
      NRF_TIMER1->TASKS_CAPTURE[0] = 1;
      NRF_TIMER1->CC[0] += delta;
      NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

      // Turn on our light
      nrf_gpio_pin_clear(currentChannel->cathode);
      nrf_gpio_pin_set(currentChannel->anode);

      nrf_gpio_cfg_output(currentChannel->cathode);
      nrf_gpio_cfg_output(currentChannel->anode);

      return ;
    }
  }
}
