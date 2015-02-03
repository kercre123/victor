#ifndef SPI_DATA_H
#define SPI_DATA_H

#include <stdint.h>
#include "hal/portable.h"

// Definition of the data structures being transferred between SYSCON and
// the vision processor

// A 16.16 fixed bit number
typedef s32 Fixed;
#define TO_FIXED(x) ((x) * 65535)
#define FIXED_MUL(x, y) ((s32)(((s64)(x) * (s64)(y)) >> 16))
#define FIXED_DIV(x, y) ((s32)(((s64)(x) << 16) / (y)))


enum SPISource
{
  SPI_SOURCE_HEAD = 'H',
  SPI_SOURCE_BODY = 'B',
  SPI_SOURCE_CLEAR = 0
};

struct GlobalCommon
{
  SPISource source;
  uint8_t RESERVED[3];
};

struct GlobalDataToHead
{
  GlobalCommon common;
  Fixed speeds[4];
  Fixed positions[4];
  Fixed IBat;
  Fixed VBat;
  Fixed Vusb;
  u8    chargeStat;
  
  uint8_t RESERVED[14];  // Pad out to 64 bytes
  char tail;
};

static_assert(sizeof(GlobalDataToHead) == 64,
  "sizeof(GlobalDataToHead) must be 64");

// TODO: get static_assert to work so we can verify sizeof

struct GlobalDataToBody
{
  GlobalCommon common;
  int16_t motorPWM[4];
  
  uint8_t RESERVED[51];  // Pad out to 64 bytes
  char tail;
};

static_assert(sizeof(GlobalDataToBody) == 64,
  "sizeof(GlobalDataToHead) must be 64");

#endif
