/**
File: imageProcessing.h
Author: Peter Barnum
Created: 2013

Definitions for imageProcessing_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
#define _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_

#include "anki/vision/robot/imageProcessing_declarations.h"

#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeXGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid() && out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeXGradient", "An input is not valid");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == imageHeight && out.get_size(1) == imageWidth,
          RESULT_FAIL_INVALID_SIZE, "ComputeXGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn = in.Pointer(y,0);

          OutType * restrict pOut = out.Pointer(y,0);

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn[x+1]) - static_cast<IntermediateType>(pIn[x-1]);
          }
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result ComputeYGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid() && out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeYGradient", "An input is not valid");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == imageHeight && out.get_size(1) == imageWidth,
          RESULT_FAIL_INVALID_SIZE, "ComputeYGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn_ym1 = in.Pointer(y-1,0);
          const InType * restrict pIn_yp1 = in.Pointer(y+1,0);

          OutType * restrict pOut = out.Pointer(y,0);

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn_yp1[x]) - static_cast<IntermediateType>(pIn_ym1[x]);
          }
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result BinomialFilter(const Array<InType> &image, Array<OutType> &imageFiltered, MemoryStack scratch)
      {
        IntermediateType kernel[BINOMIAL_FILTER_KERNEL_SIZE];
        kernel[0] = 1; kernel[1] = 4; kernel[2] = 6; kernel[3] = 4; kernel[4] = 1;
        const s32 kernelShift = 4;

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(image.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "image is not valid");

        AnkiConditionalErrorAndReturnValue(imageFiltered.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "imageFiltered is not valid");

        AnkiConditionalWarnAndReturnValue(16 == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]),
          RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

        AnkiConditionalWarnAndReturnValue(16 == (1 << kernelShift),
          RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

        AnkiConditionalErrorAndReturnValue(imageHeight == imageFiltered.get_size(0) && imageWidth == imageFiltered.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "BinomialFilter", "size(image) != size(imageFiltered) (%dx%d != %dx%d)", imageHeight, imageWidth, imageHeight, imageWidth);

        AnkiConditionalErrorAndReturnValue(image.get_rawDataPointer() != imageFiltered.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "BinomialFilter", "image and imageFiltered must be different");

        const s32 requiredScratch = imageHeight * RoundUp<s32>(imageWidth*sizeof(IntermediateType), MEMORY_ALIGNMENT);

        AnkiConditionalErrorAndReturnValue(scratch.ComputeLargestPossibleAllocation() >= requiredScratch,
          RESULT_FAIL_OUT_OF_MEMORY, "BinomialFilter", "Insufficient scratch memory");

        Array<IntermediateType> imageFilteredTmp(imageHeight, imageWidth, scratch);

        //% 1. Horizontally filter
        for(s32 y=0; y<imageHeight; y++) {
          const InType * restrict pImage = image.Pointer(y, 0);
          IntermediateType * restrict pImageFilteredTmp = imageFilteredTmp.Pointer(y, 0);

          s32 x = 0;

          pImageFilteredTmp[x] = static_cast<IntermediateType>(pImage[x])*kernel[2]   + static_cast<IntermediateType>(pImage[x+1])*kernel[3] + static_cast<IntermediateType>(pImage[x+2])*kernel[4] + static_cast<IntermediateType>(pImage[x])*(kernel[0]+kernel[1]);
          x++;
          pImageFilteredTmp[x] = static_cast<IntermediateType>(pImage[x-1])*kernel[1] + static_cast<IntermediateType>(pImage[x])*kernel[2]   + static_cast<IntermediateType>(pImage[x+1])*kernel[3] + static_cast<IntermediateType>(pImage[x+2])*kernel[4] + static_cast<IntermediateType>(pImage[x-1])*kernel[0];
          x++;

          for(; x<(imageWidth-2); x++) {
            pImageFilteredTmp[x] = static_cast<IntermediateType>(pImage[x-2])*kernel[0] + static_cast<IntermediateType>(pImage[x-1])*kernel[1] + static_cast<IntermediateType>(pImage[x])*kernel[2] + static_cast<IntermediateType>(pImage[x+1])*kernel[3] + static_cast<IntermediateType>(pImage[x+2])*kernel[4];
          }

          pImageFilteredTmp[x] = static_cast<IntermediateType>(pImage[x-2])*kernel[0] + static_cast<IntermediateType>(pImage[x-1])*kernel[1] + static_cast<IntermediateType>(pImage[x])*kernel[2] + static_cast<IntermediateType>(pImage[x+1])*kernel[3] + static_cast<IntermediateType>(pImage[x+1])*kernel[4];
          x++;
          pImageFilteredTmp[x] = static_cast<IntermediateType>(pImage[x-2])*kernel[0] + static_cast<IntermediateType>(pImage[x-1])*kernel[1] + static_cast<IntermediateType>(pImage[x])*kernel[2] + static_cast<IntermediateType>(pImage[x])*(kernel[3]+kernel[4]);
          x++;
        }

        //% 2. Vertically filter
        // for y = {0,1} unrolled
        {
          const IntermediateType * restrict pImageFilteredTmpY0 = imageFilteredTmp.Pointer(0, 0);
          const IntermediateType * restrict pImageFilteredTmpY1 = imageFilteredTmp.Pointer(1, 0);
          const IntermediateType * restrict pImageFilteredTmpY2 = imageFilteredTmp.Pointer(2, 0);
          const IntermediateType * restrict pImageFilteredTmpY3 = imageFilteredTmp.Pointer(3, 0);

          OutType * restrict pImageFiltered_y0 = imageFiltered.Pointer(0, 0);
          OutType * restrict pImageFiltered_y1 = imageFiltered.Pointer(1, 0);
          for(s32 x=0; x<imageWidth; x++) {
            const IntermediateType filtered0 = pImageFilteredTmpY0[x]*kernel[2] + pImageFilteredTmpY1[x]*kernel[3] + pImageFilteredTmpY2[x]*kernel[4] +
              pImageFilteredTmpY0[x]*(kernel[0]+kernel[1]);
            pImageFiltered_y0[x] = static_cast<OutType>(filtered0 >> (2*kernelShift));

            const IntermediateType filtered1 = pImageFilteredTmpY0[x]*kernel[1] + pImageFilteredTmpY1[x]*kernel[2] + pImageFilteredTmpY2[x]*kernel[3] + pImageFilteredTmpY3[x]*kernel[4] +
              pImageFilteredTmpY0[x]*kernel[0];
            pImageFiltered_y1[x] = static_cast<OutType>(filtered1 >> (2*kernelShift));
          }
        }

        for(s32 y=2; y<(imageHeight-2); y++) {
          const IntermediateType * restrict pImageFilteredTmpYm2 = imageFilteredTmp.Pointer(y-2, 0);
          const IntermediateType * restrict pImageFilteredTmpYm1 = imageFilteredTmp.Pointer(y-1, 0);
          const IntermediateType * restrict pImageFilteredTmpY0  = imageFilteredTmp.Pointer(y,   0);
          const IntermediateType * restrict pImageFilteredTmpYp1 = imageFilteredTmp.Pointer(y+1, 0);
          const IntermediateType * restrict pImageFilteredTmpYp2 = imageFilteredTmp.Pointer(y+2, 0);

          OutType * restrict pImageFiltered = imageFiltered.Pointer(y, 0);

          for(s32 x=0; x<imageWidth; x++) {
            const IntermediateType filtered = pImageFilteredTmpYm2[x]*kernel[0] + pImageFilteredTmpYm1[x]*kernel[1] + pImageFilteredTmpY0[x]*kernel[2] + pImageFilteredTmpYp1[x]*kernel[3] + pImageFilteredTmpYp2[x]*kernel[4];
            pImageFiltered[x] = static_cast<OutType>(filtered >> (2*kernelShift));
          }
        }

        // for y = {imageHeight-2,imageHeight-1} unrolled
        {
          const IntermediateType * restrict pImageFilteredTmpYm4 = imageFilteredTmp.Pointer(imageHeight-4, 0);
          const IntermediateType * restrict pImageFilteredTmpYm3 = imageFilteredTmp.Pointer(imageHeight-3, 0);
          const IntermediateType * restrict pImageFilteredTmpYm2 = imageFilteredTmp.Pointer(imageHeight-2, 0);
          const IntermediateType * restrict pImageFilteredTmpYm1 = imageFilteredTmp.Pointer(imageHeight-1, 0);

          OutType * restrict pImageFiltered_ym2 = imageFiltered.Pointer(imageHeight-2, 0);
          OutType * restrict pImageFiltered_ym1 = imageFiltered.Pointer(imageHeight-1, 0);
          for(s32 x=0; x<imageWidth; x++) {
            const IntermediateType filteredm1 = pImageFilteredTmpYm3[x]*kernel[0] + pImageFilteredTmpYm2[x]*kernel[1] + pImageFilteredTmpYm1[x]*kernel[2] +
              pImageFilteredTmpYm1[x]*(kernel[3]+kernel[4]);
            pImageFiltered_ym1[x] = static_cast<OutType>(filteredm1 >> (2*kernelShift));

            const IntermediateType filteredm2 = pImageFilteredTmpYm4[x]*kernel[0] + pImageFilteredTmpYm3[x]*kernel[1] + pImageFilteredTmpYm2[x]*kernel[2] + pImageFilteredTmpYm1[x]*kernel[3] +
              pImageFilteredTmpYm1[x]*kernel[4];
            pImageFiltered_ym2[x] = static_cast<OutType>(filteredm2 >> (2*kernelShift));
          }
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByTwo(const Array<InType> &image, Array<OutType> &imageDownsampled)
      {
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(image.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "DownsampleByFactor", "image is not valid");

        AnkiConditionalErrorAndReturnValue(imageDownsampled.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "DownsampleByFactor", "imageDownsampled is not valid");

        AnkiConditionalErrorAndReturnValue(imageDownsampled.get_size(0) == (imageHeight / 2) && imageDownsampled.get_size(1) == (imageWidth / 2),
          RESULT_FAIL_INVALID_SIZE, "DownsampleByFactor", "size(imageDownsampled) is not equal to size(image) >> downsampleFactor");

        const s32 maxY = 2 * imageDownsampled.get_size(0);
        const s32 maxX = 2 * imageDownsampled.get_size(1);

        for(s32 y=0, ySmall=0; y<maxY; y+=2, ySmall++) {
          const InType * restrict pImageY0 = image.Pointer(y, 0);
          const InType * restrict pImageY1 = image.Pointer(y+1, 0);

          OutType * restrict pImageDownsampled = imageDownsampled.Pointer(ySmall, 0);

          for(s32 x=0, xSmall=0; x<maxX; x+=2, xSmall++) {
            const u32 imageSum =
              static_cast<IntermediateType>(pImageY0[x]) +
              static_cast<IntermediateType>(pImageY0[x+1]) +
              static_cast<IntermediateType>(pImageY1[x]) +
              static_cast<IntermediateType>(pImageY1[x+1]);

            pImageDownsampled[xSmall] = static_cast<OutType>(imageSum >> 2);
          }
        }

        return RESULT_OK;
      }
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
