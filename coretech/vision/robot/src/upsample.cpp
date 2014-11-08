/**
File: upsample.cpp
Author: Peter Barnum
Created: 2014-10-23

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/imageProcessing.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/hostIntrinsics_m4.h"

#define ACCELERATION_NONE 0
#define ACCELERATION_ARM_M4 1
#define ACCELERATION_ARM_A7 2

#if defined(__ARM_ARCH_7A__)
#define ACCELERATION_TYPE ACCELERATION_ARM_A7
#else
#define ACCELERATION_TYPE ACCELERATION_ARM_M4
#endif

#if ACCELERATION_TYPE == ACCELERATION_NONE
#warning not using ARM acceleration
#endif

#if ACCELERATION_TYPE == ACCELERATION_ARM_A7
#include <arm_neon.h>
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      //Result UpsampleByPowerOfTwoBilinear(const Array<u8> &in, const s32 upsamplePower, Array<u8> &out, MemoryStack scratch)
      //{
      //  if(upsamplePower == 1) {
      //    return UpsampleByPowerOfTwoBilinear<1>(in, out, scratch);
      //  } else if(upsamplePower == 2) {
      //    return UpsampleByPowerOfTwoBilinear<2>(in, out, scratch);
      //  } else if(upsamplePower == 3) {
      //    return UpsampleByPowerOfTwoBilinear<3>(in, out, scratch);
      //  } else if(upsamplePower == 4) {
      //    return UpsampleByPowerOfTwoBilinear<4>(in, out, scratch);
      //  } else if(upsamplePower == 5) {
      //    return UpsampleByPowerOfTwoBilinear<5>(in, out, scratch);
      //  }

      //  AnkiError("UpsampleByPowerOfTwoBilinear", "0 < upsamplePower < 6");

      //  return RESULT_FAIL;
      //} // Result UpsampleByPowerOfTwoBilinear(const Array<u8> &in, const s32 upsamplePower, Array<u8> &out, MemoryStack scratch)
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki