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
static const uint32_t recovery_secret_code = 0x444d7852;

enum HeadToBodyFlags {
  BODY_FLASHLIGHT = 0x01
};

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
  (((w >> 10) & 0x1F) * 0x21) >> 2, \
  (((w >>  5) & 0x1F) * 0x21) >> 2, \
  (((w >>  0) & 0x1F) * 0x21) >> 2

#define UNPACK_IR(w) (w & 0x8000 ? 0xFF : 0)

#define PACK_COLORS(i,r,g,b) (  \
  (i ? 0x8000 : 0) |            \
  (((b >> 3) & 0x1F) <<  0) |   \
  (((g >> 3) & 0x1F) <<  5) |   \
  (((r >> 3) & 0x1F) << 10) |   \
)

enum SpineProtocolOp {
  NO_OPERATION,
  ASSIGN_PROP,
  SET_PROP_STATE,
  GET_PROP_STATE,
  PROP_DISCOVERED
};

__packed struct SpineProtocol {
  uint8_t opcode;

  __packed union {
    __packed struct {
      uint8_t slot;
      uint16_t colors[4];
    } SetPropState;

    __packed struct {
      uint8_t slot;
      int8_t  x, y, z;
      uint8_t shockCount;
    } GetPropState;

    __packed struct {
      uint8_t slot;
      uint32_t prop_id;
    } AssignProp;

    __packed struct {
      uint32_t prop_id;
    } PropDiscovered;

    uint8_t raw[13];
  };
} ;

static_assert(sizeof(SpineProtocol) == 14,
  "Spine protocol is incorrect size");

union GlobalDataToHead
{
  struct {
    uint32_t source;
    Fixed speeds[4];
    Fixed positions[4];
    uint32_t cliffLevel;
    Fixed VBat;
    Fixed VExt;
    u8    chargeStat;
    SpineProtocol  spineMessage;
  };

  // Force alignment
  uint8_t _RESERVED[64];
};

union GlobalDataToBody
{
  struct {
    uint32_t source;
    int16_t motorPWM[4];
    u32 backpackColors[4];
    
    SpineProtocol  spineMessage;
    uint32_t       recover;
    u8             flags;
  };

  // Force alignment
  uint8_t _RESERVED[64];  // Pad out to 64 bytes
};

static_assert(sizeof(GlobalDataToHead) + sizeof(GlobalDataToBody) <= 128,
  "Spine transmissions too long");

#endif
