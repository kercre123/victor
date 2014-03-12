/**
File: colorConversion.cpp
Author: Peter Barnum
Created: 2014-03-12

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/vision/robot/imageProcessing.h"

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      Result YUVToGrayscaleHelper(const Array<u16> &yuvImage, Array<u8> &grayscaleImage)
      {
        const s32 imageHeight = grayscaleImage.get_size(0);
        const s32 imageWidth  = grayscaleImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(yuvImage.get_size(0) == imageHeight && yuvImage.get_size(1) == imageWidth,
          RESULT_FAIL_INVALID_SIZE, "YUVToGrayscaleHelper", "inputs must be the same size");

        for(s32 y=0; y<imageHeight; y++) {
          const u16 * restrict pYuvFrame = yuvImage.Pointer(y,0);
          u8 * restrict pGrayscaleImage = grayscaleImage.Pointer(y,0);

          for(s32 x=0; x<imageWidth; x++) {
            pGrayscaleImage[x] = (pYuvFrame[x] & 0xFF00) >> 8;
          }
        }

        return RESULT_OK;
      } // namespace ImageProcessing
    } // namespace Embedded
  } // namespace Anki
