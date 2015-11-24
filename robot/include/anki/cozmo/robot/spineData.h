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
static const uint32_t secret_code = 0x444d7852;

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

struct LEDPacket {
  union {
    uint8_t ledStatus[12]; // 4-LEDs, three colors
    uint32_t secret;
  };
  uint8_t ledDark;       // Dark byte
};

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

    u8                cubeToUpdate;
    AcceleratorPacket cubeStatus;
  };

  // Force alignment
  uint8_t _RESERVED[64];
};

// TODO: get static_assert to work so we can verify sizeof

union GlobalDataToBody
{
  struct {
    uint32_t source;
    int16_t motorPWM[4];
    u32 backpackColors[4];
    
    u8          cubeToUpdate;
    LEDPacket   cubeStatus;
  };
  
  // Force alignment
  uint8_t _RESERVED[64];  // Pad out to 64 bytes
};

static_assert(sizeof(GlobalDataToHead) + sizeof(GlobalDataToBody) <= 128,
  "Spine transmissions too long");

#endif
