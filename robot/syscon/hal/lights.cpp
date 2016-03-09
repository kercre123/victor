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

static const uint16_t DivTable[] = {
	65535, 65535, 32768, 21845, 16384, 13107, 10922,  9362,
	 8192,  7281,  6553,  5957,  5461,  5041,  4681,  4369,  
	 4096,  3855,  3640,  3449,  3276,  3120,  2978,  2849, 
	 2730,  2621,  2520,  2427,  2340,  2259,  2184,  2114,  
	 2048,  1985,  1927,  1872,  1820,  1771,  1724,  1680,  
	 1638,  1598,  1560,  1524,  1489,  1456,  1424,  1394,  
	 1365,  1337,  1310,  1285,  1260,  1236,  1213,  1191,  
	 1170,  1149,  1129,  1110,  1092,  1074,  1057,  1040,  
	 1024,  1008,   992,   978,   963,   949,   936,   923,   
	  910,   897,   885,   873,   862,   851,   840,   829,   
	  819,   809,   799,   789,   780,   771,   762,   753,   
	  744,   736,   728,   720,   712,   704,   697,   689,   
	  682,   675,   668,   661,   655,   648,   642,   636,   
	  630,   624,   618,   612,   606,   601,   595,   590,   
	  585,   579,   574,   569,   564,   560,   555,   550,   
	  546,   541,   537,   532,   528,   524,   520,   516,   
	  512,   508,   504,   500,   496,   492,   489,   485,   
	  481,   478,   474,   471,   468,   464,   461,   458,   
	  455,   451,   448,   445,   442,   439,   436,   434,   
	  431,   428,   425,   422,   420,   417,   414,   412,   
	  409,   407,   404,   402,   399,   397,   394,   392,   
	  390,   387,   385,   383,   381,   378,   376,   374,   
	  372,   370,   368,   366,   364,   362,   360,   358,   
	  356,   354,   352,   350,   348,   346,   344,   343,   
	  341,   339,   337,   336,   334,   332,   330,   329,   
	  327,   326,   324,   322,   321,   319,   318,   316,   
	  315,   313,   312,   310,   309,   307,   306,   304,   
	  303,   302,   300,   299,   297,   296,   295,   293,   
	  292,   291,   289,   288,   287,   286,   284,   283,   
	  282,   281,   280,   278,   277,   276,   275,   274,   
	  273,   271,   270,   269,   268,   267,   266,   265,   
	  264,   263,   262,   261,   260,   259,   258,   257
};

static inline void AlphaBlend(
	uint8_t* color, 
	const uint16_t onColor, const uint16_t offColor, 
	const uint16_t phase, const int frames)
{
	const int coff = phase * DivTable[frames];
	const uint8_t alpha = coff >> 8;
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
	light.clock = time;

	if (delta <= 0) {
		return ;
	}
		
	switch (light.mode) {
		case TRANSITION_UP:
			AlphaBlend(light.values, state.onColor, state.offColor, light.phase, state.transitionOnFrames);
			
			if (transition(delta, light.phase, light.mode, HOLD_ON, state.transitionOnFrames)) {
				UnpackColor(light.values, state.onColor);
			}
			break ;
		case HOLD_ON:
			transition(delta, light.phase, light.mode, TRANSITION_DOWN, state.onFrames);
			break ;
		case TRANSITION_DOWN:
			AlphaBlend(light.values, state.offColor, state.onColor, light.phase, state.transitionOffFrames);
			
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
		CalculateLEDColor(lights[light++], time);
		if (light > TOTAL_LIGHTS) light = 0;
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
