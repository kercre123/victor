#ifndef SPINE_DATA_H
#define SPINE_DATA_H

#include <stdint.h>
#include "hal/portable.h"

#pragma anon_unions

// Definition of the data structures being transferred between SYSCON and
// the vision processor

// A 16.16 fixed bit number
typedef s32 Fixed;
#define TO_FIXED(x) ((x) * 65536.0 + 0.5)
#define TO_FIXED_0_32(x) ((x) * 4294967296.0 + 0.5)
#define FIXED_MUL(x, y) ((s32)(((s64)(x) * (s64)(y)) >> 16))
#define FIXED_DIV(x, y) ((s32)(((s64)(x) << 16) / (y)))

static const int spine_baud_rate    = 500000;
static const int charger_baud_rate  = 100000;

enum SPISource
{
  SPI_SOURCE_HEAD     = 0x64616568,
  SPI_SOURCE_BODY     = 0x79646f62,
  SPI_SOURCE_CRASHLOG = 0x48535243
};

// 32 bytes of payload plus tag
#define SPINE_MAX_CLAD_MSG_SIZE_DOWN (40)
#define SPINE_MAX_CLAD_MSG_SIZE_UP (32)
#define SPINE_CRASH_LOG_SIZE (sizeof(CrashLog_K02))

struct CladBufferDown
{
  uint16_t PADDING;
  uint8_t  length;
  uint8_t  msgID;
  uint8_t  data[SPINE_MAX_CLAD_MSG_SIZE_DOWN];
};

struct CladBufferUp
{
  uint16_t PADDING;
  uint8_t  length;
  uint8_t  msgID;
  uint8_t  data[SPINE_MAX_CLAD_MSG_SIZE_UP];
};

struct GlobalDataToHead
{
  uint32_t source;
  Fixed speeds[4];
  Fixed positions[4];
  int32_t cliffLevel;
  CladBufferUp cladBuffer;
};

struct GlobalDataToBody
{
  uint32_t source;
  int16_t motorPWM[4];
  CladBufferDown cladBuffer;
};

static_assert((sizeof(GlobalDataToHead) + sizeof(GlobalDataToBody)) <= 132, "Spine transport payload too large");

#endif
