#ifndef SPINE_DATA_H
#define SPINE_DATA_H

#include <stdint.h>
#include "hal/portable.h"

#pragma anon_unions

// Definition of the data structures being transferred between SYSCON and
// the vision processor

// A 16.16 fixed bit number
typedef s32 Fixed;
typedef s32 Fixed_8_24;
typedef s64 Fixed64;
#define TO_FIXED(x) ((x) * 65535.0 + 0.5)
#define TO_FIXED_8_24(x) ((x) * 16777215.0 + 0.5)
#define TO_FIXED_8_24_TO_16_16(x) ((x) >> 8)
#define FIXED_MUL(x, y) ((s32)(((s64)(x) * (s64)(y)) >> 16))
#define FIXED_DIV(x, y) ((s32)(((s64)(x) << 16) / (y)))

static const int spine_baud_rate = 350000;

enum SPISource
{
  SPI_SOURCE_HEAD    = 0x64616568,
  SPI_SOURCE_BODY    = 0x79646f62
};

struct AcceleratorPacket {
  int8_t    x, y, z;
  uint8_t   shockCount;
  uint16_t  timestamp;
};

#define UNPACK_COLORS(w) \
  UNPACK_RED(w), \
  UNPACK_GREEN(w), \
  UNPACK_BLUE(w)

#define UNPACK_RED(w)     ((((w >> 10) & 0x1F) * 0x21) >> 2)
#define UNPACK_GREEN(w)   ((((w >>  5) & 0x1F) * 0x21) >> 2)
#define UNPACK_BLUE(w)    ((((w >>  0) & 0x1F) * 0x21) >> 2)

#define UNPACK_IR(w) (w & 0x8000 ? 0xFF : 0)

#define PACK_COLORS(i,r,g,b) (  \
  (i ? 0x8000 : 0) |            \
  (((b >> 3) & 0x1F) <<  0) |   \
  (((g >> 3) & 0x1F) <<  5) |   \
  (((r >> 3) & 0x1F) << 10)     \
)

// 32 bytes of payload plus tag
#define SPINE_MAX_CLAD_MSG_SIZE (33)

struct GlobalDataToHead
{
  uint32_t source;
  Fixed speeds[4];
  Fixed positions[4];
  uint32_t cliffLevel;
  uint8_t  cladBuffer[SPINE_MAX_CLAD_MSG_SIZE];
};

struct GlobalDataToBody
{
  uint32_t source;
  int16_t motorPWM[4];
  uint8_t  cladBuffer[SPINE_MAX_CLAD_MSG_SIZE];
};

static_assert((sizeof(GlobalDataToHead) + sizeof(GlobalDataToBody)) <= 128, "Spine transport payload too large");

#endif
