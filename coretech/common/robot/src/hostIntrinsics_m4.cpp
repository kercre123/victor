/**
File: hostIntrinsics_m4.cpp
Author: Peter Barnum
Created: 2014-05-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/errorHandling.h"

#include "anki/common/vectorTypes.h"

#if USE_M4_HOST_INTRINSICS

static register32 geBits;

namespace Anki
{
  namespace Embedded
  {
    u32 __UQADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const u32 result = static_cast<u32>(register1._u8x4.v[i]) + static_cast<u32>(register2._u8x4.v[i]);
        register1._u8x4.v[i] = MIN(result, u8_MAX);
      }

      return register1;
    } // u32 __UQADD8(const u32 val1, const u32 val2)

    u32 __QADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const s32 result = static_cast<s32>(register1._s8x4.v[i]) + static_cast<s32>(register2._s8x4.v[i]);
        register1._s8x4.v[i] = CLIP(result, s8_MIN, s8_MAX);
      }

      return register1;
    } // u32 __QADD8(const u32 val1, const u32 val2)

    u32 __UQADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const u32 result = static_cast<u32>(register1._u16x2.v[i]) + static_cast<u32>(register2._u16x2.v[i]);
        register1._u16x2.v[i] = MIN(result, u16_MAX);
      }

      return register1;
    } // u32 __UQADD16(const u32 val1, const u32 val2)

    u32 __QADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const s32 result = static_cast<s32>(register1._s16x2.v[i]) + static_cast<s32>(register2._s16x2.v[i]);
        register1._s16x2.v[i] = CLIP(result, s16_MIN, s16_MAX);
      }

      return register1;
    } // u32 __QADD16(const u32 val1, const u32 val2)

    u32 __USUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<4; i++) {
        // Set the GE bits
        if(register1._u8x4.v[i] >= register2._u8x4.v[i]) {
          geBits._u8x4.v[i] = 0xFF;
        }

        // Do the operation
        register1._u8x4.v[i] = register1._u8x4.v[i] - register2._u8x4.v[i];
      }

      return register1;
    } // u32 __USUB8(const u32 val1, const u32 val2)

    u32 __SSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<4; i++) {
        // Set the GE bits
        if(register1._s8x4.v[i] >= register2._s8x4.v[i]) {
          geBits._u8x4.v[i] = 0xFF;
        }

        // Do the operation
        register1._s8x4.v[i] = register1._s8x4.v[i] - register2._s8x4.v[i];
      }

      return register1;
    } // u32 __USUB8(const u32 val1, const u32 val2)

    u32 __USUB16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<2; i++) {
        // Set the GE bits
        if(register1._u16x2.v[i] >= register2._u16x2.v[i]) {
          geBits._u16x2.v[i] = 0xFFFF;
        }

        // Do the operation
        register1._u16x2.v[i] = register1._u16x2.v[i] - register2._u16x2.v[i];
      }

      return register1;
    } // u32 __USUB16(const u32 val1, const u32 val2)

    u32 __SSUB16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<2; i++) {
        // Set the GE bits
        if(register1._s16x2.v[i] >= register2._s16x2.v[i]) {
          geBits._u16x2.v[i] = 0xFFFF;
        }

        // Do the operation
        register1._s16x2.v[i] = register1._s16x2.v[i] - register2._s16x2.v[i];
      }

      return register1;
    } // u32 __USUB16(const u32 val1, const u32 val2)

    u32 __UQSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        if(register2._u8x4.v[i] >= register1._u8x4.v[i]) {
          register1._u8x4.v[i] = 0;
        } else {
          register1._u8x4.v[i] = register1._u8x4.v[i] - register2._u8x4.v[i];
        }
      }

      return register1;
    } // u32 __UQSUB8(const u32 val1, const u32 val2)

    u32 __QSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const s32 result = static_cast<s32>(register1._s8x4.v[i]) - static_cast<s32>(register2._s8x4.v[i]);

        register1._s8x4.v[i] = CLIP(result, s8_MIN, s8_MAX);
      }

      return register1;
    } // u32 __QSUB8(const u32 val1, const u32 val2)

    u32 __UQSUB16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        if(register2._u16x2.v[i] >= register1._u16x2.v[i]) {
          register1._u16x2.v[i] = 0;
        } else {
          register1._u16x2.v[i] = register1._u16x2.v[i] - register2._u16x2.v[i];
        }
      }

      return register1;
    } // u32 __UQSUB16(const u32 val1, const u32 val2)

    u32 __QSUB16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const s32 result = static_cast<s32>(register1._s16x2.v[i]) - static_cast<s32>(register2._s16x2.v[i]);

        register1._s16x2.v[i] = CLIP(result, s16_MIN, s16_MAX);
      }

      return register1;
    } // u32 __QSUB16(const u32 val1, const u32 val2)

    s32 __SMLAD(const u32 val1, const u32 val2, const s32 accumulator)
    {
      register32 register1(val1);
      register32 register2(val2);

      s32 thisAccumulator = 0;

      for(s32 i=0; i<2; i++) {
        const s32 result = register1._s16x2.v[i] * register2._s16x2.v[i];

        thisAccumulator += result;
      }

      return accumulator + thisAccumulator;
    } // s32 __SMLAD(const u32 val1, const u32 val2, const s32 accumulator)

    u32 __USAT(const s32 val, const u8 n)
    {
      u32 result;

      AnkiAssert(n < 32);

      if(n == 31) {
        result = val;
      } else {
        const s32 maxValue = static_cast<s32>(1<<n) - 1;
        result = CLIP(val, 0, maxValue);
      }

      return result;
    } // u32 __USAT(const s32 val, const u8 n)

    s32 __SSAT(const s32 val, const u8 n)
    {
      u32 result;

      AnkiAssert(n > 0 && n < 33);

      if(n == 32) {
        result = val;
      } else {
        const s32 minValue = -(1 << (n-1));
        const s32 maxValue = (1 << (n-1)) - 1;
        result = CLIP(val, minValue, maxValue);
      }

      return result;
    } // s32 __SSAT(const s32 val, const u8 n)

    u32 __SEL(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        if(geBits._u8x4.v[i] == 0) {
          register1._u8x4.v[i] = register2._u8x4.v[i];
        }
      }

      return register1;
    } // u32 __SEL(const u32 val1, const u32 val2)
  } // namespace Embedded
} // namespace Anki

#endif // #if USE_M4_HOST_INTRINSICS
