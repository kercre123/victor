/**
File: hostIntrinsics_m4.cpp
Author: Peter Barnum
Created: 2014-05-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/errorHandling.h"

#include "coretech/common/shared/vectorTypes.h"
#include "util/math/math.h"

#if USE_M4_HOST_INTRINSICS

static register32 geBits;

namespace Anki
{
  namespace Embedded
  {
    u32 __REV(const u32 value)
    {
      register32 register1(value);
      register32 register2;

      register2._u8x4.v[0] = register1._u8x4.v[3];
      register2._u8x4.v[1] = register1._u8x4.v[2];
      register2._u8x4.v[2] = register1._u8x4.v[1];
      register2._u8x4.v[3] = register1._u8x4.v[0];

      return register2;
    } // u32 __REV(const u32 value)

    u32 __REV16(const u32 value)
    {
      register32 register1(value);
      register32 register2;

      register2._u8x4.v[0] = register1._u8x4.v[1];
      register2._u8x4.v[1] = register1._u8x4.v[0];
      register2._u8x4.v[2] = register1._u8x4.v[3];
      register2._u8x4.v[3] = register1._u8x4.v[2];

      return register2;
    } // u32 __REV16(const u32 value)

    s32 __REVSH(const s16 value)
    {
      register32 register1(value);
      register32 register2(0);

      register2._u8x4.v[0] = register1._u8x4.v[1];
      register2._u8x4.v[1] = register1._u8x4.v[0];

      if(register2._u32 & (1<<15)) {
        register2._u8x4.v[2] = 0xFF;
        register2._u8x4.v[3] = 0xFF;
      }

      return register2;
    } // s32 __REVSH(const s16 value)

    u32 __RBIT(const u32 value)
    {
      u32 bitFlipped = 0;

      for(s32 i=0; i<32; i++) {
        const u32 curBit = !!(value & (1<<i));

        bitFlipped |= (curBit << (31-i));
      }

      return bitFlipped;
    } // u32 __RBIT(const u32 value)

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

    u8 __CLZ(const u32 value)
    {
      u8 numZeros = 0;

      for(s32 i=31; i>=0; i--) {
        if(value & (1<<i)) {
          break;
        } else {
          numZeros++;
        }
      }

      return numZeros;
    } // u8 __CLZ(const u32 value)

    u32 __SADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<4; i++) {
        s32 sum = static_cast<s32>(register1._s8x4.v[i]) + static_cast<s32>(register2._s8x4.v[i]);

        // Set the GE bits
        if(sum >= 0) {
          geBits._u8x4.v[i] = 0xFF;
        }

        if(sum < std::numeric_limits<s8>::min()) {
          sum += 0x100;
        } else if(sum > std::numeric_limits<s8>::max()) {
          sum -= 0x100;
        }

        AnkiAssert(CLIP(sum, static_cast<s32>(std::numeric_limits<s8>::min()), static_cast<s32>(std::numeric_limits<s8>::max())) == sum);

        register1._s8x4.v[i] = static_cast<s8>(sum);
      }

      return register1;
    } // u32 __SADD8(const u32 val1, const u32 val2)

    u32 __QADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const s32 result = static_cast<s32>(register1._s8x4.v[i]) + static_cast<s32>(register2._s8x4.v[i]);
        register1._s8x4.v[i] = CLIP(result, static_cast<s32>(std::numeric_limits<s8>::min()), static_cast<s32>(std::numeric_limits<s8>::max()));
      }

      return register1;
    } // u32 __QADD8(const u32 val1, const u32 val2)

    u32 __SHADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        s32 sum = static_cast<s32>(register1._s8x4.v[i]) + static_cast<s32>(register2._s8x4.v[i]);

        sum >>= 1;

        AnkiAssert(CLIP(sum, static_cast<s32>(std::numeric_limits<s8>::min()), static_cast<s32>(std::numeric_limits<s8>::max())) == sum);

        register1._s8x4.v[i] = static_cast<s8>(sum);
      }

      return register1;
    } // u32 __SHADD8(const u32 val1, const u32 val2)

    u32 __UADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<4; i++) {
        u32 sum = static_cast<u32>(register1._u8x4.v[i]) + static_cast<u32>(register2._u8x4.v[i]);

        // Set the GE bits
        if(sum > std::numeric_limits<u8>::max()) {
          geBits._u8x4.v[i] = 0xFF;
          sum -= 0x100;
        }

        AnkiAssert(CLIP(sum, static_cast<u32>(0), static_cast<u32>(std::numeric_limits<u8>::max())) == sum);

        register1._u8x4.v[i] = static_cast<u8>(sum);
      }

      return register1;
    } // u32 __UADD8(const u32 val1, const u32 val2)

    u32 __UQADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const u32 result = static_cast<u32>(register1._u8x4.v[i]) + static_cast<u32>(register2._u8x4.v[i]);
        register1._u8x4.v[i] = MIN(result, static_cast<u32>(std::numeric_limits<u8>::max()));
      }

      return register1;
    } // u32 __UQADD8(const u32 val1, const u32 val2)

    u32 __UHADD8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        u32 sum = static_cast<u32>(register1._u8x4.v[i]) + static_cast<u32>(register2._u8x4.v[i]);

        sum >>= 1;

        AnkiAssert(sum <= std::numeric_limits<u8>::max());

        register1._u8x4.v[i] = static_cast<u8>(sum);
      }

      return register1;
    } // u32 __UHADD8(const u32 val1, const u32 val2)

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
    } // u32 __SSUB8(const u32 val1, const u32 val2)

    u32 __QSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        const s32 result = static_cast<s32>(register1._s8x4.v[i]) - static_cast<s32>(register2._s8x4.v[i]);

        register1._s8x4.v[i] = CLIP(result, static_cast<s32>(std::numeric_limits<s8>::min()), static_cast<s32>(std::numeric_limits<s8>::max()));
      }

      return register1;
    } // u32 __QSUB8(const u32 val1, const u32 val2)

    u32 __SHSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        s32 result = static_cast<s32>(register1._s8x4.v[i]) - static_cast<s32>(register2._s8x4.v[i]);

        result >>= 1;

        AnkiAssert(CLIP(result, static_cast<s32>(std::numeric_limits<s8>::min()), static_cast<s32>(std::numeric_limits<s8>::max())) == result);

        register1._s8x4.v[i] = static_cast<s8>(result);
      }

      return register1;
    } // u32 __SHSUB8(const u32 val1, const u32 val2)

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

    u32 __UHSUB8(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<4; i++) {
        s32 result = static_cast<s32>(register1._u8x4.v[i]) - static_cast<s32>(register2._u8x4.v[i]);

        result >>= 1;

        if(result < 0) {
          result += 0x100;
        }

        AnkiAssert(result <= std::numeric_limits<u8>::max());

        register1._u8x4.v[i] = static_cast<u8>(result);
      }

      return register1;
    } // u32 __UHSUB8(const u32 val1, const u32 val2)

    u32 __SADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<2; i++) {
        s32 sum = static_cast<s32>(register1._s16x2.v[i]) + static_cast<s32>(register2._s16x2.v[i]);

        // Set the GE bits
        if(sum >= 0) {
          geBits._u16x2.v[i] = 0xFFFF;
        }

        if(sum < std::numeric_limits<s16>::min()) {
          sum += 0x10000;
        } else if(sum > std::numeric_limits<s16>::max()) {
          sum -= 0x10000;
        }

        AnkiAssert(CLIP(sum, static_cast<s32>(std::numeric_limits<s16>::min()), static_cast<s32>(std::numeric_limits<s16>::max())) == sum);

        register1._s16x2.v[i] = static_cast<s16>(sum);
      }

      return register1;
    } // u32 __SADD16(const u32 val1, const u32 val2)

    u32 __QADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const s32 result = static_cast<s32>(register1._s16x2.v[i]) + static_cast<s32>(register2._s16x2.v[i]);
        register1._s16x2.v[i] = CLIP(result, static_cast<s32>(std::numeric_limits<s16>::min()), static_cast<s32>(std::numeric_limits<s16>::max()));
      }

      return register1;
    } // u32 __QADD16(const u32 val1, const u32 val2)

    u32 __SHADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        s32 sum = static_cast<s32>(register1._s16x2.v[i]) + static_cast<s32>(register2._s16x2.v[i]);

        sum >>= 1;

        AnkiAssert(CLIP(sum, static_cast<s32>(std::numeric_limits<s16>::min()), static_cast<s32>(std::numeric_limits<s16>::max())) == sum);

        register1._s16x2.v[i] = static_cast<s16>(sum);
      }

      return register1;
    } // u32 __SHADD16(const u32 val1, const u32 val2);

    u32 __UADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      geBits._u32 = 0;

      for(s32 i=0; i<2; i++) {
        u32 sum = static_cast<u32>(register1._u16x2.v[i]) + static_cast<u32>(register2._u16x2.v[i]);

        // Set the GE bits
        if(sum > std::numeric_limits<u16>::max()) {
          geBits._u16x2.v[i] = 0xFFFF;
          sum -= 0x10000;
        }

        AnkiAssert(CLIP(sum, static_cast<u32>(0), static_cast<u32>(std::numeric_limits<u16>::max())) == sum);

        register1._u16x2.v[i] = static_cast<u16>(sum);
      }

      return register1;
    } //  u32 __UADD16(const u32 val1, const u32 val2);

    u32 __UQADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const u32 result = static_cast<u32>(register1._u16x2.v[i]) + static_cast<u32>(register2._u16x2.v[i]);
        register1._u16x2.v[i] = MIN(result, static_cast<u32>(std::numeric_limits<u16>::max()));
      }

      return register1;
    } // u32 __UQADD16(const u32 val1, const u32 val2)

    u32 __UHADD16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        u32 sum = static_cast<u32>(register1._u16x2.v[i]) + static_cast<u32>(register2._u16x2.v[i]);

        sum >>= 1;

        AnkiAssert(sum <= std::numeric_limits<u16>::max());

        register1._u16x2.v[i] = static_cast<u16>(sum);
      }

      return register1;
    } // u32 __UHADD16(const u32 val1, const u32 val2)

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
    } // u32 __SSUB16(const u32 val1, const u32 val2)

    u32 __QSUB16(const u32 val1, const u32 val2)
    {
      register32 register1(val1);
      register32 register2(val2);

      for(s32 i=0; i<2; i++) {
        const s32 result = static_cast<s32>(register1._s16x2.v[i]) - static_cast<s32>(register2._s16x2.v[i]);

        register1._s16x2.v[i] = CLIP(result, static_cast<s32>(std::numeric_limits<s16>::min()), static_cast<s32>(std::numeric_limits<s16>::max()));
      }

      return register1;
    } // u32 __QSUB16(const u32 val1, const u32 val2)

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

    // TODO: verify implementation before using
    //u32 __UASX(const u32 val1, const u32 val2)
    //{
    //  register32 register1(val1);
    //  register32 register2(val2);

    //  register1._u16x2.hi = register1._u16x2.hi + register2._u16x2.lo;
    //  register1._u16x2.lo = register1._u16x2.lo - register2._u16x2.hi;

    //  // NOT SURE IF THESE ARE SET (Documentation is inconsistent)
    //  geBits._u16x2.hi = register1._u16x2.hi >= 0x10000 ? 0xFFFF : 0;
    //  geBits._u16x2.lo = register1._u16x2.lo >= 0 ? 0xFFFF : 0;

    //  return register1;
    //} //u32 __UASX(const u32 val1, const u32 val2)

    // TODO: verify implementation before using
    //u32 __USAX(const u32 val1, const u32 val2)
    //{
    //  register32 register1(val1);
    //  register32 register2(val2);

    //  register1._u16x2.hi = register1._u16x2.hi - register2._u16x2.lo;
    //  register1._u16x2.lo = register1._u16x2.lo + register2._u16x2.hi;

    //  // NOT SURE IF THESE ARE SET (Documentation is inconsistent)
    //  geBits._u16x2.hi = register1._u16x2.hi >= 0 ? 0xFFFF : 0;
    //  geBits._u16x2.lo = register1._u16x2.lo >= 0x10000 ? 0xFFFF : 0;

    //  return register1;
    //} // u32 __USAX(const u32 val1, const u32 val2)

    // TODO: verify implementation before using
    //u32 __SASX(const u32 val1, const u32 val2)
    //{
    //  register32 register1(val1);
    //  register32 register2(val2);

    //  register1._s16x2.hi = register1._s16x2.hi + register2._s16x2.lo;
    //  register1._s16x2.lo = register1._s16x2.lo - register2._s16x2.hi;

    //  // NOT SURE IF THESE ARE SET (Documentation is inconsistent)
    //  geBits._s16x2.hi = register1._s16x2.hi >= 0 ? 0xFFFF : 0;
    //  geBits._s16x2.lo = register1._s16x2.lo >= 0 ? 0xFFFF : 0;

    //  return register1;
    //} //u32 __SASX(const u32 val1, const u32 val2)

    // TODO: verify implementation before using
    //u32 __SSAX(const u32 val1, const u32 val2)
    //{
    //  register32 register1(val1);
    //  register32 register2(val2);

    //  register1._s16x2.hi = register1._s16x2.hi - register2._s16x2.lo;
    //  register1._s16x2.lo = register1._s16x2.lo + register2._s16x2.hi;

    //  // NOT SURE IF THESE ARE SET (Documentation is inconsistent)
    //  geBits._s16x2.hi = register1._s16x2.hi >= 0 ? 0xFFFF : 0;
    //  geBits._s16x2.lo = register1._s16x2.lo >= 0 ? 0xFFFF : 0;

    //  return register1;
    //} // u32 __SSAX(const u32 val1, const u32 val2)

    // TODO: verify implementation before using
    //u32 __USAD8(const u32 val1, const u32 val2)
    //{
    //  register32 register1(val1);
    //  register32 register2(val2);

    //  for(s32 i=0; i<4; i++) {
    //    // Do the operation
    //    register1._u8x4.v[i] = ABS(register1._u8x4.v[i] - register2._u8x4.v[i]);
    //  }

    //  return register1._u8x4.v[0] + register1._u8x4.v[1] +
    //    register1._u8x4.v[2] + register1._u8x4.v[3];
    //} // u32 __USAD8(const u32 val1, const u32 val2);

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
