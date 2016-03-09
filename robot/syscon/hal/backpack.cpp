#include <string.h>

#include "backpack.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "radio.h"

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
  {PIN_LED1, {PIN_LED2, PIN_LED3, PIN_LED4}},
  {PIN_LED2, {PIN_LED1, PIN_LED3, PIN_LED4}},
  {PIN_LED3, {PIN_LED1, PIN_LED2, PIN_LED4}},
  {PIN_LED4, {PIN_LED1, PIN_LED3, PIN_LED2}}
};

// Charlieplexing magic constants
static const int numChannels = 4;
static const int numLightsPerChannel = 3;
static const int numLights = numChannels*numLightsPerChannel;

static uint8_t pdm_value[numLights];
static int channel = 0;

static void lights_off() {
  nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}

// Start all pins as input
void Backpack::init()
{
  lights_off();
	RTOS::schedule(manage);
}

void Backpack::update(void) {
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
    
		int pin = RGBLightPins[channel].cathodes[i];
    if (overflow > 0xFF) {
      nrf_gpio_pin_clear(pin);
      nrf_gpio_cfg_output(pin);
    }
  }
  
  channel = (channel + 1) % numChannels;
}

void Backpack::manage(void*) {
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
    6,  7,  8,
    3,  4,  5,
    9, 10, 11,
    2,  1,  1,
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
		uint8_t* levels = Lights::state(BACKPACK_LIGHT_INDEX_BASE+g);
		
		for (int i = 0; i < 3; i++, idx++) {		
			pdm_value[index[idx]] = AdjustTable[(gamma[idx] * levels[i]) >> 10];
		}
	}
}

void Backpack::setLights(const LightState* update) {
  for (int i = 0; i < NUM_BACKPACK_LEDS; i++) {
		Lights::update(i, &update[i]);
	}
}
