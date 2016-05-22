#ifndef __LED_H
#define __LED_H

#include <stdint.h>

#include "radio.h"

#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

#define UNPACK_COLORS(w) \
  UNPACK_RED(w), \
  UNPACK_GREEN(w), \
  UNPACK_BLUE(w)

#define UNPACK_RED(w)     ((((w >> 10) & 0x1F) * 0x21) >> 2)
#define UNPACK_GREEN(w)   ((((w >>  5) & 0x1F) * 0x21) >> 2)
#define UNPACK_BLUE(w)    ((((w >>  0) & 0x1F) * 0x21) >> 2)
#define UNPACK_IR(w)      (w & 0x8000 ? 0xFF : 0)

#define PACK_RED(r)       (((r >> 3) & 0x1F) << 10)
#define PACK_GREEN(g)     (((g >> 3) & 0x1F) <<  5)
#define PACK_BLUE(b)      (((b >> 3) & 0x1F) <<  0)
#define PACK_IR(i)        (i ? 0x8000 : 0)

#define PACK_COLORS(i,r,g,b) (  \
   PACK_RED(r) | \
   PACK_GREEN(g) | \
   PACK_BLUE(b) | \
   PACK_IR(i) \
)

static const int LIGHTS_PER_WORD = 4;

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

union ControllerLights {
  struct {
    LightValues cube[MAX_ACCESSORIES][NUM_PROP_LIGHTS];
    LightValues backpack[5];
  };
  
  LightValues lights[];
};

static const int TOTAL_LIGHTS = sizeof(ControllerLights) / sizeof(LightValues);

extern ControllerLights lightController;

namespace Lights {
  void init();
  void manage();
  void update(LightValues& light, const LightState* ledParams);
};

#endif
