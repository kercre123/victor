#include <string.h>

#include "led.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"

#include "debug.h"

#include "rtos.h"
#include "hardware.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/spineData.h"

using namespace Anki::Cozmo;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Define charlie wiring here:
static const charliePlex_s RGBLightPins[] =
{
  // anode, cath_red, cath_gree, cath_blue
  {PIN_LED1, {PIN_LED2, PIN_LED3, PIN_LED4}},
  {PIN_LED2, {PIN_LED1, PIN_LED3, PIN_LED4}},
  {PIN_LED3, {PIN_LED1, PIN_LED2, PIN_LED4}},
  {PIN_LED4, {PIN_LED1, PIN_LED3, PIN_LED2}}
};

// Charlieplexing magic constants
static const int numChannels = 4;
static const int numLightsPerChannel = 3;
static const int numLights = numChannels*numLightsPerChannel;

static LightState lights[NUM_BACKPACK_LEDS];
static uint32_t phases[NUM_BACKPACK_LEDS];

static uint8_t pdm_value[numLights];
static int channel = 0;

static void lights_off() {
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

// Start all pins as input
void Lights::init()
{
  lights_off();
  
  memset(lights, 0, sizeof(lights));

  RTOS::schedule(manage);
}

void Lights::update(void) {
  #ifndef RADIO_TIMING_TEST
  static uint8_t pwm[numLights];

  // Blacken everything out
  lights_off();

  // Setup anode
  nrf_gpio_pin_set(RGBLightPins[channel].anode);
  nrf_gpio_cfg_output(RGBLightPins[channel].anode);
  
  // Set lights for current charlie channel
  for (int i = 0, index = channel * numLightsPerChannel; i < numLightsPerChannel; i++, index++)
  {
    int overflow = pwm[index] + (int)pdm_value[index];
    pwm[index] = overflow;
    
    if (overflow > 0xFF) {
      nrf_gpio_pin_clear(RGBLightPins[channel].cathodes[i]);
      nrf_gpio_cfg_output(RGBLightPins[channel].cathodes[i]);
    }
  }
  
  channel = (channel + 1) % numChannels;
  #endif
}

void Lights::manage(void*) {
  // Channel 1 is unused by the light mapping
  static const int8_t index[][3] = { 
    {  0,  1,  1 },
    {  6,  7,  8 },
    {  3,  4,  5 },
    {  9, 10, 11 },
    {  2,  1,  1 }
  };

  static const uint8_t* gamma[][3] = { 
    { AdjustTableFull,             NULL,            NULL },
    {  AdjustTableRed, AdjustTableGreen, AdjustTableFull },
    {  AdjustTableRed, AdjustTableGreen, AdjustTableFull },
    {  AdjustTableRed, AdjustTableGreen, AdjustTableFull },
    { AdjustTableFull,             NULL,            NULL },                      
  };

  uint32_t time = GetFrame();
  
  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
    uint8_t levels[4];
    CalculateLEDColor(levels, lights[i], time, phases[i]);
    
    for (int c = 0; c < 3; c++) {
      pdm_value[index[i][c]] = gamma[i][c][levels[c]];
    }
  }
}

void Lights::setLights(const LightState* update) {
  memcpy(lights, update, sizeof(lights));
  memset(phases, 0, sizeof(phases));
}
