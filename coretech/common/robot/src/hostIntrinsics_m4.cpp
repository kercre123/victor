/**
File: hostIntrinsics_m4.cpp
Author: Peter Barnum
Created: 2014-05-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/vectorTypes.h"

#if USE_M4_HOST_INTRINSICS

static register32 geBits;

namespace Anki
{
  namespace Embedded
  {
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
