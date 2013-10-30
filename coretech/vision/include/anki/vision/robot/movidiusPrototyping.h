#ifndef _ANKICORETECHEMBEDDED_VISION_MOVIDIUS_PROTOTYPING_H_
#define _ANKICORETECHEMBEDDED_VISION_MOVIDIUS_PROTOTYPING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "anki/common/robot/config.h"
#include "anki/vision/robot/cInterfaces_vision_c.h"

  void ScrollingIntegralImage_u8_s32_FilterRow_innerLoop(const s32 * restrict integralImage_00, const s32 * restrict integralImage_01, const s32 * restrict integralImage_10, const s32 * restrict integralImage_11, const s32 minX, const s32 maxX, const s32 imageWidth, s32 * restrict pOutput);

  // pOutput must point to a buffer with at least sizeof(s32) bytes.
  void ScrollingIntegralImage_u8_s32_FilterRow_innerLoop_emulateShave(const s32 * restrict integralImage_00, const s32 * restrict integralImage_01, const s32 * restrict integralImage_10, const s32 * restrict integralImage_11, const s32 minX, const s32 maxX, const s32 imageWidth, s32 * restrict pOutput);

  // This might be used later, but not yet
  //s32 ScrollingIntegralImage_u8_s32_FilterRow(
  //  const C_ScrollingIntegralImage_u8_s32 * restrict integralImage,
  //  const C_Rectangle_s16 * restrict filter,
  //  const s32 imageRow,
  //  C_Array_s32 * restrict output);

#ifdef __cplusplus
}
#endif

#endif //_ANKICORETECHEMBEDDED_VISION_MOVIDIUS_PROTOTYPING_H_
