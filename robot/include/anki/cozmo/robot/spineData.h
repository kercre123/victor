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
  SPI_SOURCE_BODY    = 0x79646f62,
  SPI_ENTER_RECOVERY = 0x444d7852
};

// 32 bytes of payload plus tag
#define SPINE_MAX_CLAD_MSG_SIZE_DOWN (41)
#define SPINE_MAX_CLAD_MSG_SIZE_UP (29)

typedef enum {
  SF_WiFi_Connected = 0x01,
  SF_BLE_Connected  = 0x02,
  SF_OBJ_Connected  = 0x04,
} SpineFlags;

struct CladBufferDown
{
  uint16_t flags;
  uint8_t  length;
  uint8_t  data[SPINE_MAX_CLAD_MSG_SIZE_DOWN];
};

struct CladBufferUp
{
  uint16_t flags;
  uint8_t  length;
  uint8_t  data[SPINE_MAX_CLAD_MSG_SIZE_UP];
};

struct GlobalDataToHead
{
  uint32_t source;
  Fixed speeds[4];
  Fixed positions[4];
  uint32_t cliffLevel;
  CladBufferUp cladBuffer;
};

struct GlobalDataToBody
{
  uint32_t source;
  int16_t motorPWM[4];
  CladBufferDown cladBuffer;
};

static_assert((sizeof(GlobalDataToHead) + sizeof(GlobalDataToBody)) <= 128, "Spine transport payload too large");

#endif
