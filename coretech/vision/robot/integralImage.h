/**
File: integralImage.h
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_
#define _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d.h"

#include "coretech/vision/robot/integralImage_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename OutType> NO_INLINE Result ScrollingIntegralImage_u8_s32::FilterRow(const Rectangle<s16> &filter, const s32 imageRow, const s32 outputMultiply, const s32 outputRightShift, Array<OutType> &output) const
    {
      AnkiAssert(AreValid(*this, output));

      // TODO: support these
      AnkiAssert(filter.left <= 0);
      AnkiAssert(filter.top <= 0);
      AnkiAssert(filter.right >= 0);
      AnkiAssert(filter.bottom >= 0);

      const bool insufficientBorderPixels_left = (-filter.left+1) > this->numBorderPixels;
      const bool insufficientBorderPixels_right = (filter.right) > this->numBorderPixels;

      // These min and max coordinates are in the original image's coordinate frame
      const s32 minX = insufficientBorderPixels_left ? -(this->numBorderPixels + filter.left - 1) : 0;
      const s32 maxX = insufficientBorderPixels_right ? (this->get_size(1) - 1 - filter.right + this->numBorderPixels) : (this->get_imageWidth()-1);

      // Get the four pointers to the corners of the filter in the integral image.
      // The x offset is added at the end, because it might be invalid for x==0.
      // The -1 terms are because the rectangular sums should be inclusive.
      const s32 topOffset = imageRow - this->rowOffset + filter.top - 1 ;
      const s32 bottomOffset = imageRow - this->rowOffset + filter.bottom;
      const s32 leftOffset = filter.left - 1 + this->numBorderPixels;
      const s32 rightOffset = filter.right + this->numBorderPixels;

      const s32 * restrict pIntegralImage_00 = this->Pointer(topOffset, 0) + leftOffset;
      const s32 * restrict pIntegralImage_01 = this->Pointer(topOffset, 0) + rightOffset;
      const s32 * restrict pIntegralImage_10 = this->Pointer(bottomOffset, 0) + leftOffset;
      const s32 * restrict pIntegralImage_11 = this->Pointer(bottomOffset, 0) + rightOffset;

      OutType * restrict pOutput = output.Pointer(0,0);

      if(minX > 0)
        memset(pOutput, 0, minX*sizeof(OutType));

      ScrollingIntegralImage_u8_s32::FilterRow_innerLoop(
        minX, maxX,
        outputMultiply, outputRightShift,
        pIntegralImage_00, pIntegralImage_01, pIntegralImage_10, pIntegralImage_11,
        pOutput);

      if((maxX+1) < imageWidth)
        memset(pOutput+maxX+1, 0, (imageWidth - (maxX+1))*sizeof(OutType));

      return RESULT_OK;
    }

    template<typename OutType> void ScrollingIntegralImage_u8_s32::FilterRow_innerLoop(
      const s32 minX,
      const s32 maxX,
      const s32 outputMultiply,
      const s32 outputRightShift,
      const s32 * restrict pIntegralImage_00,
      const s32 * restrict pIntegralImage_01,
      const s32 * restrict pIntegralImage_10,
      const s32 * restrict pIntegralImage_11,
      OutType * restrict pOutput)
    {
      if(outputMultiply == 1 && outputRightShift == 0) {
        for(s32 x=minX; x<=maxX; x++) {
          pOutput[x] = static_cast<OutType>( pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] );
        }
      } else {
        for(s32 x=minX; x<=maxX; x++) {
          pOutput[x] = static_cast<OutType>( ((pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x]) * outputMultiply) >> outputRightShift ) ;
        }
      }
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_
