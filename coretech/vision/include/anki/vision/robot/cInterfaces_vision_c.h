#ifndef _ANKICORETECHEMBEDDED_VISION_C_INTERFACES_H_
#define _ANKICORETECHEMBEDDED_VISION_C_INTERFACES_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/cInterfaces_c.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct {
    //
    // Array members
    //
    s32 size0, size1;
    s32 stride;
    s32 flags;

    s32 * restrict data;

    //
    // ScrollingIntegralImage members
    //
    s32 imageWidth; //< width of the original image
    s32 maxRow;
    s32 rowOffset; //< Row 0 of this integral image corresponds to which row of the original image (can be negative)
    s32 numBorderPixels;
  } C_ScrollingIntegralImage_u8_s32;

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_VISION_C_INTERFACES_H_