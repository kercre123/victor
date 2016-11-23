#ifndef __LED_H
#define __LED_H

#include <stdint.h>

#include "cubes.h"
#include "backpack.h"

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

static const int MAX_KEYFRAMES = 8;

enum LightMode {
  SOLID = 0,
  HOLD,
  FADE
};

struct LightSet {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t ir;
};

struct Keyframe {
  uint16_t color;
  uint8_t holdFrames;
  uint8_t transitionFrames;
};

struct LightValues {
  // Color palette
  Keyframe keyframes[MAX_KEYFRAMES];

  // Operating mode
  LightMode mode;
  LightSet values;

  // Phase info
  int phase;
  int index;
  int frameCount;
};

union ControllerLights {
  struct {
    LightValues cube[MAX_ACCESSORIES][NUM_PROP_LIGHTS];
    LightValues backpack[BACKPACK_LIGHTS];
  };
  
  LightValues lights[];
};

static const int TOTAL_LIGHTS = sizeof(ControllerLights) / sizeof(LightValues);

extern ControllerLights lightController;

namespace Lights {
  void init();
  void manage();
  void update(LightValues& light, const LightState* ledParams);
  void setHeadlight(bool status);
};

#endif
