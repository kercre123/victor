/**
File: integralImage.h
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_
#define _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

#include "anki/vision/robot/integralImage_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename OutType> NO_INLINE Result ScrollingIntegralImage_u8_s32::FilterRow(const Rectangle<s16> &filter, const s32 imageRow, const s32 outputMultiply, const s32 outputRightShift, Array<OutType> &output) const
    {
      AnkiAssert(this->IsValid());
      AnkiAssert(output.IsValid());

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

      // Various SIMD versions for different data type sizes
      if(sizeof(OutType) == 1) {
        if(outputMultiply == 1 && outputRightShift == 0) {
          const s32 maxX_firstCycles = RoundUp(minX, 4);

          for(s32 x=minX; x<maxX_firstCycles; x++) {
            pOutput[x] = static_cast<OutType>( pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] );
          }

          u32 * restrict pOutputU32 = reinterpret_cast<u32*>(output.Pointer(0,0));

          const s32 minX_simd4 = maxX_firstCycles / 4;
          const s32 maxX_simd4 = maxX / 4;
          for(s32 x=minX_simd4; x<=maxX_simd4; x++) {
            const u32 out0 = static_cast<OutType>( pIntegralImage_11[x*4] - pIntegralImage_10[x*4] + pIntegralImage_00[x*4] - pIntegralImage_01[x*4] );
            const u32 out1 = static_cast<OutType>( pIntegralImage_11[x*4+1] - pIntegralImage_10[x*4+1] + pIntegralImage_00[x*4+1] - pIntegralImage_01[x*4+1] );
            const u32 out2 = static_cast<OutType>( pIntegralImage_11[x*4+2] - pIntegralImage_10[x*4+2] + pIntegralImage_00[x*4+2] - pIntegralImage_01[x*4+2] );
            const u32 out3 = static_cast<OutType>( pIntegralImage_11[x*4+3] - pIntegralImage_10[x*4+3] + pIntegralImage_00[x*4+3] - pIntegralImage_01[x*4+3] );

            pOutputU32[x] = (out0 & 0xFF) | ((out1 & 0xFF) << 8) | ((out2 & 0xFF) << 16) | ((out3 & 0xFF) << 24);
          }
        } else {
          const s32 maxX_firstCycles = RoundUp(minX, 4);

          for(s32 x=minX; x<maxX_firstCycles; x++) {
            pOutput[x] = static_cast<OutType>( pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] );
          }

          u32 * restrict pOutputU32 = reinterpret_cast<u32*>(output.Pointer(0,0));

          const s32 minX_simd4 = maxX_firstCycles / 4;
          const s32 maxX_simd4 = maxX / 4;
          for(s32 x=minX_simd4; x<=maxX_simd4; x++) {
            const u32 out0 = static_cast<OutType>( ((pIntegralImage_11[x*4] - pIntegralImage_10[x*4] + pIntegralImage_00[x*4] - pIntegralImage_01[x*4]) * outputMultiply) >> outputRightShift );
            const u32 out1 = static_cast<OutType>( ((pIntegralImage_11[x*4+1] - pIntegralImage_10[x*4+1] + pIntegralImage_00[x*4+1] - pIntegralImage_01[x*4+1]) * outputMultiply) >> outputRightShift );
            const u32 out2 = static_cast<OutType>( ((pIntegralImage_11[x*4+2] - pIntegralImage_10[x*4+2] + pIntegralImage_00[x*4+2] - pIntegralImage_01[x*4+2]) * outputMultiply) >> outputRightShift );
            const u32 out3 = static_cast<OutType>( ((pIntegralImage_11[x*4+3] - pIntegralImage_10[x*4+3] + pIntegralImage_00[x*4+3] - pIntegralImage_01[x*4+3]) * outputMultiply) >> outputRightShift );

            pOutputU32[x] = (out0 & 0xFF) | ((out1 & 0xFF) << 8) | ((out2 & 0xFF) << 16) | ((out3 & 0xFF) << 24);
          }
        }
      } else {
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

      if((maxX+1) < imageWidth)
        memset(pOutput+maxX+1, 0, (imageWidth - (maxX+1))*sizeof(OutType));

      return RESULT_OK;
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_H_
