#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "rtos.h"
#include "timer.h"
#include "lights.h"
#include "backpack.h"
#include "radio.h"

enum LightMode {
	HOLD_VALUE,
	TRANSITION_UP,
	HOLD_ON,
	TRANSITION_DOWN,
	HOLD_OFF
};

struct LightValues {
	LightState state;
	LightMode mode;
	int clock;
	int phase;
	uint8_t values[LIGHTS_PER_WORD];
};

static LightValues lights[TOTAL_LIGHTS];

static inline void AlphaBlend(uint8_t* color, const uint16_t onColor, const uint16_t offColor, const uint16_t alpha)
{
  const uint8_t invAlpha = ~alpha;

  const uint8_t onRed  = UNPACK_RED(onColor);
  const uint8_t onGrn  = UNPACK_GREEN(onColor);
  const uint8_t onBlu  = UNPACK_BLUE(onColor);
  const uint8_t offRed = UNPACK_RED(offColor);
  const uint8_t offGrn = UNPACK_GREEN(offColor);
  const uint8_t offBlu = UNPACK_BLUE(offColor);

  color[0] = (onRed * alpha + offRed * invAlpha) >> 8;
  color[1] = (onGrn * alpha + offGrn * invAlpha) >> 8;
  color[2] = (onBlu * alpha + offBlu * invAlpha) >> 8;
  color[3] = UNPACK_IR(alpha >= 0x80 ? onColor : offColor);
}

static inline void UnpackColor(uint8_t* rgbi, uint16_t newColor) {
  uint8_t colors[] = { UNPACK_COLORS(newColor) };
  memcpy(rgbi, colors, sizeof(colors));
}

static inline bool transition(const int time, int& phase, LightMode& mode, const LightMode next, const uint8_t frames) {
	phase += time;
	if (phase >= frames) {
		phase -= frames;
		mode = next;
		return true;
	}
	return false;
}

static void CalculateLEDColor(LightValues& light, const uint32_t time)
{	
	const LightState& state = light.state;
	
	int delta = time - light.clock;

	if (delta <= 0) {
		return ;
	}

	light.clock = time;
		
	switch (light.mode) {
		case TRANSITION_UP:
			AlphaBlend(light.values, state.onColor, state.offColor, light.phase * 0x100 / state.transitionOnFrames);
			if (transition(delta, light.phase, light.mode, HOLD_ON, state.transitionOnFrames)) {
				UnpackColor(light.values, state.onColor);
			}
			break ;
		case HOLD_ON:
			transition(delta, light.phase, light.mode, TRANSITION_DOWN, state.onFrames);
			break ;
		case TRANSITION_DOWN:
			AlphaBlend(light.values, state.offColor, state.onColor, light.phase * 0x100 / state.transitionOffFrames);
			if (transition(delta, light.phase, light.mode, HOLD_OFF, state.transitionOffFrames)) {
				UnpackColor(light.values, state.offColor);
			}
			break ;
		case HOLD_OFF:
			transition(delta, light.phase, light.mode, TRANSITION_UP, state.offFrames);
			break ;
		default:
			return ;
	}
}

void Lights::init() {
	memset(lights, 0, sizeof(lights));
	
  // Spread light processing across the radio period
	RTOS_Task* task = RTOS::schedule(Lights::manage);
	RTOS::setPriority(task, RTOS_LOW_PRIORITY);
}

void Lights::manage(void* userdata) {
  static int light = 0;
	int time = GetFrame();
	
	for (int i = 0; i < 4; i++) {
		CalculateLEDColor(lights[light], time);
		light = (light + 1) % TOTAL_LIGHTS;
	}
}

void Lights::update(int index, const LightState* params) {
	LightValues* light = &lights[index];
	
	memcpy(&light->state, params, sizeof(LightState));

	// If this is a constant light
  if (params->onFrames == 255 || (params->onColor == params->offColor)) {
		light->mode = HOLD_VALUE;		
    UnpackColor(light->values, params->onColor);
  } else {
		light->mode = TRANSITION_UP;
		light->phase = 0;
	}
}

uint8_t* Lights::state(int idx) {
	return lights[idx].values;
}
