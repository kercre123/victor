/**
File: shaveKernels_vision_c.h
Author: Peter Barnum
Created: 2013

C header for the SHAVE vision kernels

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_SHAVE_KERNELS_H_
#define _ANKICORETECHEMBEDDED_VISION_SHAVE_KERNELS_H_

#include "anki/common/robot/config.h"

#ifdef __cplusplus
extern "C" {
#endif

  void ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow(
    const u8 * restrict paddedImage_currentRow,
    const s32 * restrict integralImage_previousRow,
    s32 * restrict integralImage_currentRow,
    const s32 integralImageWidth
    );

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_VISION_SHAVE_KERNELS_H_