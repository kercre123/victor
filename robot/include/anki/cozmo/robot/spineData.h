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


enum SPISource
{
  SPI_SOURCE_HEAD = 'H',
  SPI_SOURCE_BODY = 'B',
  SPI_SOURCE_CLEAR = 0
};

union GlobalCommon
{
  struct {
    SPISource source;
    uint8_t SYNC[3];
  };
  uint32_t common;
};

struct AcceleratorPacket {
  int8_t    x, y, z;
  uint8_t   shockCount;
  uint16_t  timestamp;
};

struct LEDPacket {
  uint8_t ledStatus[12]; // 4-LEDs, three colors
  uint8_t ledDark;       // Dark byte
};

union GlobalDataToHead
{
  struct {
    GlobalCommon common;
    Fixed speeds[4];
    Fixed positions[4];
    Fixed IBat; // This no longer exists here
    Fixed VBat;
    Fixed VExt;
    u8    chargeStat;

    u8                cubeToUpdate;
    AcceleratorPacket cubeStatus;

    uint8_t _TAIL;
  };

  // Force alignment
  struct {
    uint8_t _RESERVED[63];
    uint8_t tail;
  };
};

static_assert(sizeof(GlobalDataToHead) == 64,
  "sizeof(GlobalDataToHead) must be 64");

// TODO: get static_assert to work so we can verify sizeof

union GlobalDataToBody
{
  struct {
    GlobalCommon common;
    int16_t motorPWM[4];
    u32 backpackColors[4];
    
    u8          cubeToUpdate;
    LEDPacket   cubeStatus;
    
    uint8_t _TAIL;
  };
  
  // Force alignment
  struct {
    uint8_t _RESERVED[63];  // Pad out to 64 bytes
    uint8_t tail;
  };
};

static_assert(sizeof(GlobalDataToBody) == 64,
  "sizeof(GlobalDataToHead) must be 64");

#endif
