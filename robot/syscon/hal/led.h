#ifndef __LED_H
#define __LED_H

#include <stdint.h>
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

extern const uint8_t AdjustTableFull[];
extern const uint8_t AdjustTableRed[];
extern const uint8_t AdjustTableGreen[];

void CalculateLEDColor(uint8_t* rgbi,
                       const LightState& ledParams,
                       const uint32_t currentTime,
                       uint32_t& phaseTime);

#endif
