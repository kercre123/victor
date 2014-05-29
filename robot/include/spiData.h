#ifndef SPI_DATA_H
#define SPI_DATA_H

#include <stdint.h>
#include "hal/portable.h"

// Definition of the data structures being transferred between SYSCON and
// the vision processor

typedef s32 Fixed;

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
  
  uint8_t RESERVED[27];  // Pad out to 64 bytes
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
