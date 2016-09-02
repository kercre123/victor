#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "timer.h"
#include "lights.h"
#include "backpack.h"
#include "cubes.h"

ControllerLights lightController;

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
  LightSet& color, 
  const uint16_t startColor, const uint16_t endColor,
  const uint16_t phase, const int frames)
{
  const unsigned int alpha = phase * DivTable[frames];
  const unsigned int invAlpha = 0x10000 - alpha;

  const LightSet start = { UNPACK_COLORS(startColor) };
  const LightSet end = { UNPACK_COLORS(endColor) };
  
  color.red = (start.red * invAlpha + end.red * alpha) >> 16;
  color.green = (start.green * invAlpha + end.green * alpha) >> 16;
  color.blue = (start.blue * invAlpha + end.blue * alpha) >> 16;
  color.ir = (alpha < 0x8000) ? start.ir : end.ir;
}

static inline bool transition(int& phase, LightMode& mode, const uint16_t frames, const LightMode next) {
  if (phase >= frames) {
    phase -= frames;
    mode = next;
    return true;
  }
  return false;
}

static void CalculateLEDColor(LightValues& light, const uint32_t delta)
{ 
  light.phase += delta;

  // Advance light
  for (;;) {
    // Current keyframe
    const Keyframe* now = &light.keyframes[light.index];
    
    switch (light.mode) {
      case SOLID:
        return ;

      case HOLD:
        if (transition(light.phase, light.mode, now->holdFrames, FADE)) {
          continue ;
        }

        return ;

      case FADE:
        // Next frame
        int next_frame = light.index + 1;
        if (next_frame >= light.frameCount) {
          next_frame = 0;
        }

        // Next keyframe
        const Keyframe* then = &light.keyframes[next_frame];

        // Transition to a hold phase
        if (transition(light.phase, light.mode, now->transitionFrames, HOLD)) {
          light.index = next_frame;
          
          LightSet color = { UNPACK_COLORS(then->color) };
          memcpy(&light.values, &color, sizeof(color));

          continue ;
        }

        AlphaBlend(light.values, now->color, then->color, light.phase, now->transitionFrames);

        return ;
    }
  }
}

void Lights::init() {  
  // Set all our lights to off
  memset(&lightController, 0, sizeof(lightController));
}

void Lights::manage() {
  static unsigned int last_counter = 0;
  static unsigned int accumulator = 0;

  unsigned int counter = GetCounter();
  unsigned int delta = counter - last_counter;
  last_counter = counter;

  accumulator += delta * FRAME_RATE;

  int frame_delta = 0;
  while (accumulator >= TICKS_PER_SECOND) {
    frame_delta++;
    accumulator -= TICKS_PER_SECOND;
  }

  if (frame_delta <= 0) {
    return ;
  }

  for(int i = 0; i < TOTAL_LIGHTS; ++i)
  {
    CalculateLEDColor(lightController.lights[i], frame_delta);
  }
}

void Lights::update(LightValues& light, const LightState* params) {
  if (params->onColor == params->offColor) {
    light.mode = SOLID;
  } else {
    light.mode = HOLD;
    light.index = 0;
    // offset + offFrames so we start in the fade on phase
    light.phase = params->offset + params->offFrames;
    light.frameCount = 2;

    light.keyframes[0].color = params->offColor;
    light.keyframes[0].holdFrames = params->offFrames;
    light.keyframes[0].transitionFrames = params->transitionOnFrames;

    light.keyframes[1].color = params->onColor;
    light.keyframes[1].holdFrames = params->onFrames;
    light.keyframes[1].transitionFrames = params->transitionOffFrames;
  }

  // Preset the color
  LightSet color = { UNPACK_COLORS(params->offColor) };
  memcpy(&light.values, &color, sizeof(color));
}
