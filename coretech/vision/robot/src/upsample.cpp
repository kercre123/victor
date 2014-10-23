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

template<> void UpsampleByPowerOfTwoBilinear_innerLoop<1>(
  const u8 * restrict pInY0,
  const u8 * restrict pInY1,
  Anki::Embedded::Array<u8> &out,
  const s32 ySmall,
  const s32 smallWidth,
  const s32 outStride)
{
  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++) {
    const u8 smallUL = pInY0[xSmall];
    const u8 smallUR = pInY0[xSmall+1];
    const u8 smallLL = pInY1[xSmall];
    const u8 smallLR = pInY1[xSmall+1];

    u8 * restrict pOut = out.Pointer(ySmall*2 + 2/2, 0);

    const s32 xBig0 = xSmall*2 + 2/2;

    for(s32 dy=0; dy<2; dy++) {
      const u8 alpha = 2*2 - 2*dy - 1;
      const u8 alphaInverse = 2*dy + 1;

      const u16 interpolatedPixelL0 = smallUL * alpha;
      const u16 interpolatedPixelL1 = smallLL * alphaInverse;
      const u16 interpolatedPixelL = interpolatedPixelL0 + interpolatedPixelL1;
      const u16 subtractAmount = interpolatedPixelL >> (1-1);

      const u16 interpolatedPixelR0 = smallUR * alpha;
      const u16 interpolatedPixelR1 = smallLR * alphaInverse;
      const u16 interpolatedPixelR = interpolatedPixelR0 + interpolatedPixelR1;
      const u16 addAmount = interpolatedPixelR >> (1-1);

      u16 curValue = 2*interpolatedPixelL + ((addAmount - subtractAmount)>>1);

      for(s32 dx=0; dx<2; dx++) {
        const u8 curValueU8 = curValue >> (1+2);

        pOut[xBig0 + dx] = curValueU8;

        curValue += addAmount - subtractAmount;
      } // for(s32 dx=0; dx<2; dx++)

      pOut += outStride;
    } // for(s32 dy=0; dy<2; dy++)
  } //  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++)
} // UpsampleByPowerOfTwoBilinear_innerLoop<1>()

template<> void UpsampleByPowerOfTwoBilinear_innerLoop<2>(
  const u8 * restrict pInY0,
  const u8 * restrict pInY1,
  Anki::Embedded::Array<u8> &out,
  const s32 ySmall,
  const s32 smallWidth,
  const s32 outStride)
{
  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++) {
    const u8 smallUL = pInY0[xSmall];
    const u8 smallUR = pInY0[xSmall+1];
    const u8 smallLL = pInY1[xSmall];
    const u8 smallLR = pInY1[xSmall+1];

    u8 * restrict pOut = out.Pointer(ySmall*4 + 4/2, 0);

    const s32 xBig0 = xSmall*4 + 4/2;

    for(s32 dy=0; dy<4; dy++) {
      const u8 alpha = 2*4 - 2*dy - 1;
      const u8 alphaInverse = 2*dy + 1;

      const u16 interpolatedPixelL0 = smallUL * alpha;
      const u16 interpolatedPixelL1 = smallLL * alphaInverse;
      const u16 interpolatedPixelL = interpolatedPixelL0 + interpolatedPixelL1;
      const u16 subtractAmount = interpolatedPixelL >> (2-1);

      const u16 interpolatedPixelR0 = smallUR * alpha;
      const u16 interpolatedPixelR1 = smallLR * alphaInverse;
      const u16 interpolatedPixelR = interpolatedPixelR0 + interpolatedPixelR1;
      const u16 addAmount = interpolatedPixelR >> (2-1);

      u16 curValue = 2*interpolatedPixelL + ((addAmount - subtractAmount)>>1);

      for(s32 dx=0; dx<4; dx++) {
        const u8 curValueU8 = curValue >> (2+2);

        pOut[xBig0 + dx] = curValueU8;

        curValue += addAmount - subtractAmount;
      } // for(s32 dx=0; dx<4; dx++)

      pOut += outStride;
    } // for(s32 dy=0; dy<4; dy++)
  } //  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++)
} // UpsampleByPowerOfTwoBilinear_innerLoop<2>()

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki