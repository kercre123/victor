/**
File: hostIntrinsics_m4.h
Author: Peter Barnum
Created: 2014-05-22

This host intrinsics functions allow for compilation of ARM m4 intrinsics on any platform.
This set is incomplete, and there is no guarantee of correctness.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_
#define _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_

#include "anki/common/robot/config.h"

#if !USE_M4_HOST_INTRINSICS

#ifdef USING_CHIP_SIMULATOR
#include <ARMCM4.h>
#else
#include <stm32f4xx.h>
#endif

#else

namespace Anki
{
  namespace Embedded
  {
    u32 __USUB8(const u32 val1, const u32 val2);
    u32 __SSUB8(const u32 val1, const u32 val2);

    u32 __USUB16(const u32 val1, const u32 val2);
    u32 __SSUB16(const u32 val1, const u32 val2);

    u32 __SEL(const u32 val1, const u32 val2);
  } // namespace Embedded
} //namespace Anki

#endif // #if USE_M4_HOST_INTRINSICS

#endif // _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_
