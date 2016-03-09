#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "rtos.h"
#include "timer.h"
#include "lights.h"
#include "backpack.h"
#include "radio.h"

static LightState lightStates[TOTAL_LIGHTS];
static uint32_t lightPhases[TOTAL_LIGHTS];
static uint8_t lightValues[TOTAL_LIGHTS][LIGHTS_PER_WORD];

static inline void AlphaBlend(uint8_t* color, const uint16_t onColor, const uint16_t offColor, const uint16_t alpha)
{
  const uint8_t onRed  = UNPACK_RED(onColor);
  const uint8_t onGrn  = UNPACK_GREEN(onColor);
  const uint8_t onBlu  = UNPACK_BLUE(onColor);
  const uint8_t offRed = UNPACK_RED(offColor);
  const uint8_t offGrn = UNPACK_GREEN(offColor);
  const uint8_t offBlu = UNPACK_BLUE(offColor);
  const uint16_t invAlpha = 0x100 - alpha;

  color[0] = (onRed * alpha + offRed * invAlpha) >> 8;
  color[1] = (onGrn * alpha + offGrn * invAlpha) >> 8;
  color[2] = (onBlu * alpha + offBlu * invAlpha) >> 8;
  color[3] = UNPACK_IR(alpha >= 0x80 ? onColor : offColor);
}

static inline void UnpackColor(uint8_t* rgbi, uint16_t newColor) {
  uint8_t colors[] = { UNPACK_COLORS(newColor) };
  memcpy(rgbi, colors, sizeof(colors));
}

static void CalculateLEDColor(uint8_t* rgbi,
                       const LightState& ledParams,
                       const uint32_t currentTime,
                       uint32_t& phaseTime)
{
  // Constant color
  if (ledParams.onFrames == 255 || (ledParams.onColor == ledParams.offColor)) {
    UnpackColor(rgbi, ledParams.onColor);
    return ;
  }

  uint32_t phase = currentTime - phaseTime;
  
  if (phase <= ledParams.transitionOnFrames) // Still turning on
  {
    AlphaBlend(rgbi, ledParams.onColor, ledParams.offColor, phase * 0x100 / ledParams.transitionOnFrames);
    return ;
  }
  else if (phase <= (ledParams.transitionOnFrames + ledParams.onFrames))
  {
    UnpackColor(rgbi, ledParams.onColor);
    return ;
  }
  else if (phase <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames))
  {
    const uint32_t offPhase = phase - (ledParams.transitionOnFrames + ledParams.onFrames);
    AlphaBlend(rgbi, ledParams.offColor, ledParams.onColor, offPhase * 0x100 / ledParams.transitionOffFrames);
    return ;
  }
  else if (phase <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offFrames))
  {
    UnpackColor(rgbi, ledParams.offColor);
    return ;
  }

  UnpackColor(rgbi, ledParams.offColor);
  phaseTime = currentTime;
}

void Lights::init() {
	memset(lightStates, 0, sizeof(lightStates));
	
  // Spread light processing across the radio period
	RTOS_Task* task = RTOS::schedule(Lights::manage);
	RTOS::setPriority(task, RTOS_LOW_PRIORITY);
}

void Lights::manage(void* userdata) {
  static int light = 0;
	int time = GetFrame();
	
	for (int i = 0; i < 4; i++) {
		CalculateLEDColor(lightValues[light], lightStates[light], time, lightPhases[light]);
		light = (light + 1) % TOTAL_LIGHTS;
	}
}

void Lights::update(int index, const LightState* ledParams) {
	memcpy(&lightStates[index], ledParams, sizeof(LightState));
	lightPhases[index] = 0;
}

uint8_t* Lights::state(int idx) {
	return lightValues[idx];
}
