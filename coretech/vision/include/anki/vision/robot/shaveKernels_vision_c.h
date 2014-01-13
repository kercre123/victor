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
#include "anki/common/robot/shaveKernels_c.h"

#pragma mark --- Natural C declarations ---

#ifdef __cplusplus
extern "C" {
#endif

  DECLARE_SHAVE_FUNCTION(ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow,
  (
    const u8 * restrict paddedImage_currentRow,
    const s32 * restrict integralImage_previousRow,
    s32 * restrict integralImage_currentRow,
    const s32 integralImageWidth
    ));

  DECLARE_SHAVE_FUNCTION(ScrollingIntegralImage_u8_s32_FilterRow,
  (
    const s32 * restrict pIntegralImage_00,
    const s32 * restrict pIntegralImage_01,
    const s32 * restrict pIntegralImage_10,
    const s32 * restrict pIntegralImage_11,
    const s32 minX,
    const s32 maxX,
    const s32 imageWidth,
    s32 * restrict pOutput
    ));

  void ScrollingIntegralImage_u8_s32_FilterRow(
    const s32 * restrict pIntegralImage_00,
    const s32 * restrict pIntegralImage_01,
    const s32 * restrict pIntegralImage_10,
    const s32 * restrict pIntegralImage_11,
    const s32 minX,
    const s32 maxX,
    const s32 imageWidth,
    s32 * restrict pOutput
    );

#ifdef __cplusplus
}
#endif

#pragma mark --- Per-Shave C declarations ---
// TODO: Does this have to be done by hand?

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_SHAVE_0
  void shave0_ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow(
    const u8 * restrict paddedImage_currentRow,
    const s32 * restrict integralImage_previousRow,
    s32 * restrict integralImage_currentRow,
    const s32 integralImageWidth
    );
#endif

#ifdef USE_SHAVE_0
  void shave0_ScrollingIntegralImage_u8_s32_FilterRow(
    const s32 * restrict pIntegralImage_00,
    const s32 * restrict pIntegralImage_01,
    const s32 * restrict pIntegralImage_10,
    const s32 * restrict pIntegralImage_11,
    const s32 minX,
    const s32 maxX,
    const s32 imageWidth,
    s32 * restrict pOutput
    );
#endif

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_VISION_SHAVE_KERNELS_H_