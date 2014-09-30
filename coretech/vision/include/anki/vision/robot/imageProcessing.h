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
#include "anki/common/robot/interpolate.h"

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/comparisons.h"

//#include "anki/common/robot/matlabInterface.h"

#include "anki/common/robot/hostIntrinsics_m4.h"

#define USE_ARM_ACCELERATION_IMAGE_PROCESSING

#ifndef USE_ARM_ACCELERATION_IMAGE_PROCESSING
#warning not using USE_ARM_ACCELERATION_IMAGE_PROCESSING
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      // Helper for several fixed-point filtering operations
      // Compute the direction and number of bits to shift
      void GetBitShiftDirectionAndMagnitude(
        const s32 in1_numFractionalBits,
        const s32 in2_numFractionalBit,
        const s32 out_numFractionalBit,
        s32 &shiftMagnitude,
        bool &shiftRight);

      template<typename InType, typename IntermediateType, typename OutType> Result ComputeXGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "ComputeXGradient", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(in, out),
          RESULT_FAIL_INVALID_SIZE, "ComputeXGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn = in.Pointer(y,0);

          OutType * restrict pOut = out.Pointer(y,0);

          // Fill in left boundary
          pOut[0] = 0;

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn[x+1]) - static_cast<IntermediateType>(pIn[x-1]);
          }

          // Fill in right boundary
          pOut[imageWidth-1] = 0;
        }

        // Fill in top/bottom boundaries
        OutType * restrict pOutTop = out.Pointer(0,0);
        OutType * restrict pOutBtm = out.Pointer(imageHeight-1,0);
        for(s32 x=0; x<imageWidth; x++) {
          pOutTop[x] = 0;
          pOutBtm[x] = 0;
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result ComputeYGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "ComputeYGradient", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(in, out),
          RESULT_FAIL_INVALID_SIZE, "ComputeYGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn_ym1 = in.Pointer(y-1,0);
          const InType * restrict pIn_yp1 = in.Pointer(y+1,0);

          OutType * restrict pOut = out.Pointer(y,0);

          // Fill in left boundary
          pOut[0] = 0;

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn_yp1[x]) - static_cast<IntermediateType>(pIn_ym1[x]);
          }

          // Fill in right boundary
          pOut[imageWidth-1] = 0;
        }

        // Fill in top/bottom boundaries
        OutType * restrict pOutTop = out.Pointer(0,0);
        OutType * restrict pOutBtm = out.Pointer(imageHeight-1,0);
        for(s32 x=0; x<imageWidth; x++) {
          pOutTop[x] = 0;
          pOutBtm[x] = 0;
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

        AnkiConditionalErrorAndReturnValue(AreValid(image, imageFiltered, scratch),
          RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "Invalid objects");

        AnkiConditionalWarnAndReturnValue(16 == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]),
          RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

        AnkiConditionalWarnAndReturnValue(16 == (1 << kernelShift),
          RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

        AnkiConditionalErrorAndReturnValue(imageHeight == imageFiltered.get_size(0) && imageWidth == imageFiltered.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "BinomialFilter", "size(image) != size(imageFiltered) (%dx%d != %dx%d)", imageHeight, imageWidth, imageHeight, imageWidth);

        AnkiConditionalErrorAndReturnValue(NotAliased(image, imageFiltered),
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

      template<typename InType, typename OutType>
      Result CreateIntegralImage(const Array<InType> &image, Array<OutType> integralImage)
      {
        AnkiConditionalErrorAndReturnValue(AreValid(image, integralImage),
          RESULT_FAIL_INVALID_OBJECT,
          "ImageProcessing::CreateIntegralImage",
          "Invalid objects");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth  = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, integralImage),
          RESULT_FAIL_INVALID_SIZE,
          "ImageProcessing::CreateIntegralImage",
          "Output integralImage array must match input image's size.");

        // Fill in first row of integral image
        const InType * restrict pImageRow         = image.Pointer(0,0);
        OutType      * restrict pIntegralImageRow = integralImage.Pointer(0,0);

        pIntegralImageRow[0] = static_cast<OutType>(pImageRow[0]);
        for(s32 x=1; x<imageWidth; x++) {
          pIntegralImageRow[x] = pIntegralImageRow[x-1] + static_cast<OutType>(pImageRow[x]);
        }

        // Fill in remaining rows of integral image
        for(s32 y=1; y<imageHeight; y++) {
          const InType * restrict pImageRow             = image.Pointer(y,0);
          OutType      * restrict pIntegralImageRow     = integralImage.Pointer(y,0);
          OutType      * restrict pIntegralImageRowPrev = integralImage.Pointer(y-1,0);

          pIntegralImageRow[0] = static_cast<f32>(pImageRow[0]) + pIntegralImageRowPrev[0];
          for(s32 x=1; x<imageWidth; x++) {
            pIntegralImageRow[x] = (pIntegralImageRow[x-1] + pIntegralImageRowPrev[x] +
              static_cast<OutType>(pImageRow[x]) - pIntegralImageRowPrev[x-1]);
          }
        }

        return RESULT_OK;
      } // CreateIntegralImage()

      template<typename InType, typename IntermediateType, typename OutType> Result BoxFilter(const Array<InType> &image, const s32 boxHeight, const s32 boxWidth, Array<OutType> &filtered, MemoryStack scratch)
      {
        AnkiConditionalErrorAndReturnValue(image.IsValid() && filtered.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "BoxFilter", "Image is invalid");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth  = image.get_size(1);

        const s32 boxHeight2 = boxHeight / 2;
        const s32 boxWidth2 = boxWidth / 2;

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, filtered),
          RESULT_FAIL_INVALID_SIZE, "BoxFilter", "Output normalized image must match input image's size.");

        AnkiConditionalErrorAndReturnValue(boxHeight > 2 && boxWidth > 2 && IsOdd(boxWidth) && IsOdd(boxHeight),
          RESULT_FAIL_INVALID_SIZE, "BoxFilter", "Box filter must be greater than two and odd");

        s32 y;

        // Includes extra padding for simd
        IntermediateType * restrict verticalAccumulator = reinterpret_cast<IntermediateType*>( scratch.Allocate(imageWidth*sizeof(IntermediateType) + 16) );
        memset(verticalAccumulator, 0, imageWidth*sizeof(IntermediateType));

        // Accumulate a whole boxHeight
        for(y=0; y<boxHeight; y++) {
          const InType * restrict pImage = image.Pointer(y,0);
          for(s32 x=0; x<imageWidth; x++) {
            verticalAccumulator[x] += pImage[x];
          }
        }

        //
        // Add the first row to the filtered image
        //

        filtered(0,boxHeight2-1,0,-1).Set(0);

        {
          // Grab the pointer to the horizontally negative-offset location in the filtered image
          OutType * restrict pFiltered = filtered.Pointer(boxHeight2,0) - boxWidth2;

          IntermediateType horizontalAccumulator = 0;

          s32 x;
          for(x=0; x<boxWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x];
          }

          filtered(boxHeight2,boxHeight2,0,boxWidth2-1).Set(0);

          pFiltered[x-1] = horizontalAccumulator;

          for(; x<imageWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x] - verticalAccumulator[x-boxWidth];
            pFiltered[x] = horizontalAccumulator;
          }

          filtered(boxHeight2,boxHeight2,-boxWidth2,-1).Set(0);
        }

        //
        // Add the remaining rows to the filtered image
        //

        for(; y<imageHeight; y++) {
          // Grab the pointer to the horizontally negative-offset location in the filtered image
          OutType * restrict pFiltered = filtered.Pointer(y - boxHeight2,0) - boxWidth2;

          const InType * restrict pImageOld = image.Pointer(y-boxHeight,0);
          const InType * restrict pImageNew = image.Pointer(y,0);

          for(s32 x=0; x<imageWidth; x++) {
            verticalAccumulator[x] += pImageNew[x];
            verticalAccumulator[x] -= pImageOld[x];
          }

          IntermediateType horizontalAccumulator = 0;

          s32 x;
          for(x=0; x<boxWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x];
          }

          filtered(y-boxHeight2,y-boxHeight2,0,boxWidth2-1).Set(0);

          pFiltered[x-1] = horizontalAccumulator;

          for(; x<imageWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x] - verticalAccumulator[x-boxWidth];
            pFiltered[x] = horizontalAccumulator;
          }

          filtered(y-boxHeight2,y-boxHeight2,-boxWidth2,-1).Set(0);
        }

        filtered(-boxHeight2,-1,0,-1).Set(0);

        return RESULT_OK;
      } // Result BoxFilter()

      template<typename InType, typename OutType> Result Resize(const Array<InType> &in, Array<OutType> &out)
      {
        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "Resize", "Invalid objects");

        const s32 inHeight = in.get_size(0);
        const s32 inWidth  = in.get_size(1);

        const s32 outHeight = out.get_size(0);
        const s32 outWidth  = out.get_size(1);

        Point<f32> scale(
          static_cast<f32>(inWidth)  / static_cast<f32>(outWidth),
          static_cast<f32>(inHeight) / static_cast<f32>(outHeight));

        const f32 yInStart     = (0.5f * scale.y) - 0.5f;
        const f32 yInIncrement = scale.y;

        const f32 xInStart     = (0.5f * scale.x) - 0.5f;
        const f32 xInIncrement = scale.x;

        for(s32 y=0; y<outHeight; y++) {
          const f32 inY = yInStart + yInIncrement * static_cast<f32>(y);

          s32 inY0_S32 = FloorS32(inY);
          s32 inY1_S32 = CeilS32(inY);

          // Technically, we can't interpolate the borders. But this is a reasonable approximation
          if(inY0_S32 < 0)
            inY0_S32 = 0;

          if(inY1_S32 < 0)
            inY1_S32 = 0;

          if(inY0_S32 > (inHeight-1))
            inY0_S32 = inHeight-1;

          if(inY1_S32 > (inHeight-1))
            inY1_S32 = inHeight-1;

          const f32 inY0 = static_cast<f32>(inY0_S32);
          //const f32 inY1 = static_cast<f32>(inY1_S32);

          const f32 alphaY = inY - inY0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const InType * restrict pIn_y0 = in.Pointer(inY0_S32, 0);
          const InType * restrict pIn_y1 = in.Pointer(inY1_S32, 0);

          OutType * restrict pOut = out.Pointer(y, 0);

          for(s32 x=0; x<outWidth; x++) {
            const f32 inX = xInStart + xInIncrement * static_cast<f32>(x);

            s32 inX0_S32 = FloorS32(inX);
            s32 inX1_S32 = CeilS32(inX);

            // Technically, we can't interpolate the borders. But this is a reasonable approximation
            if(inX0_S32 < 0)
              inX0_S32 = 0;

            if(inX1_S32 < 0)
              inX1_S32 = 0;

            if(inX0_S32 > (inWidth-1))
              inX0_S32 = inWidth-1;

            if(inX1_S32 > (inWidth-1))
              inX1_S32 = inWidth-1;

            const f32 inX0 = static_cast<f32>(inX0_S32);
            //const f32 inX1 = static_cast<f32>(inX1_S32);

            const f32 alphaX = inX - inX0;
            const f32 alphaXinverse = 1.0f - alphaX;

            const f32 pixelTL = static_cast<f32>(pIn_y0[inX0_S32]);
            const f32 pixelTR = static_cast<f32>(pIn_y0[inX1_S32]);
            const f32 pixelBL = static_cast<f32>(pIn_y1[inX0_S32]);
            const f32 pixelBR = static_cast<f32>(pIn_y1[inX1_S32]);

            const f32 interpolatedPixelValueF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

            pOut[x] = RoundIfInteger<OutType>(interpolatedPixelValueF32);
          } // for(s32 x=0; x<outWidth; x++)
        } // for(s32 y=0; y<outHeight; y++)

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByTwo(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "DownsampleByFactor", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(inHeight / 2, inWidth / 2, out),
          RESULT_FAIL_INVALID_SIZE, "DownsampleByFactor", "size(out) is not equal to size(in) >> downsampleFactor");

        const s32 maxY = 2 * out.get_size(0);
        const s32 maxX = 2 * out.get_size(1);

        for(s32 y=0, ySmall=0; y<maxY; y+=2, ySmall++) {
          const InType * restrict pImageY0 = in.Pointer(y, 0);
          const InType * restrict pImageY1 = in.Pointer(y+1, 0);

          OutType * restrict pImageDownsampled = out.Pointer(ySmall, 0);

          for(s32 x=0, xSmall=0; x<maxX; x+=2, xSmall++) {
            const u32 inSum =
              static_cast<IntermediateType>(pImageY0[x]) +
              static_cast<IntermediateType>(pImageY0[x+1]) +
              static_cast<IntermediateType>(pImageY1[x]) +
              static_cast<IntermediateType>(pImageY1[x+1]);

            pImageDownsampled[xSmall] = static_cast<OutType>(inSum >> 2);
          }
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByPowerOfTwo(const Array<InType> &in, const s32 downsamplePower, Array<OutType> &out, MemoryStack scratch)
      {
        const s32 largeHeight = in.get_size(0);
        const s32 largeWidth = in.get_size(1);

        const s32 smallHeight = largeHeight >> downsamplePower;
        const s32 smallWidth = largeWidth >> downsamplePower;

        const s32 downsampleFactor = 1 << downsamplePower;

        AnkiConditionalErrorAndReturnValue(AreValid(in, out , scratch),
          RESULT_FAIL_INVALID_OBJECT, "DownsampleByFactor", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(smallHeight, smallWidth, out),
          RESULT_FAIL_INVALID_SIZE, "DownsampleByFactor", "size(out) is not equal to size(in) >> downsampleFactor");

        AnkiConditionalErrorAndReturnValue(largeWidth % 4 == 0,
          RESULT_FAIL_INVALID_SIZE, "DownsampleByFactor", "The width of the in Array must be a multiple of four");

        Array<InType> inRow(1, largeWidth, scratch);
        Array<IntermediateType> accumulator(1, largeWidth >> downsamplePower, scratch);

        InType * restrict pInRow = inRow.Pointer(0,0);
        IntermediateType * restrict pAccumulator = accumulator.Pointer(0,0);

        const s32 numWordsToCopy = (sizeof(InType)*largeWidth) >> 2; // If the input in stride is not a multiple of four, this will be too small

        const s32 normalizationShift = 2*downsamplePower;

        for(s32 ySmall=0; ySmall<smallHeight; ySmall++) {
          accumulator.SetZero();

          // Accumulate a block of "largeWidth X downsampleFactor" pixels into a "smallWidth X 1" buffer
          for(s32 yp=0; yp<downsampleFactor; yp++) {
            const s32 yLarge = (ySmall << downsamplePower) + yp; // The actual row of the input image

            const InType * restrict pIn = in.Pointer(yLarge, 0);

            // First, copy a row from in to the temporary buffer
            // TODO: DMA may be faster
            for(s32 i=0; i<numWordsToCopy; i++) {
              AnkiAssert(reinterpret_cast<size_t>(pIn) % 4 == 0);
              reinterpret_cast<u32*>(pInRow)[i] = reinterpret_cast<const u32*>(pIn)[i];
            }

            // Next, accumulate into the accumulator
            for(s32 xSmall=0; xSmall<smallWidth; xSmall++) {
              for(s32 xp=0; xp<downsampleFactor; xp++) {
                const s32 xLarge = (xSmall << downsamplePower) + xp; // The actual column of the input image

                pAccumulator[xSmall] += static_cast<IntermediateType>(pInRow[xLarge]);
              } // for(s32 xp=0; xp<downsampleFactor; xp++)
            } // for(s32 xSmall=0; xSmall<smallWidth; xSmall++)
          } // for(s32 yp=0; yp<downsampleFactor; yp++)

          // Convert the sums to averages
          OutType * restrict pOut = out.Pointer(ySmall, 0);
          for(s32 xSmall=0; xSmall<smallWidth; xSmall++) {
            pOut[xSmall] = static_cast<OutType>(pAccumulator[xSmall] >> normalizationShift);
          } // for(s32 xSmall=0; xSmall<smallWidth; xSmall++)
        } // for(s32 ySmall=0; ySmall<smallHeight; ySmall++)

        return RESULT_OK;
      }

      template<typename Type> FixedPointArray<Type> Get1dGaussianKernel(const s32 sigma, const s32 numSigmaFractionalBits, const s32 numStandardDeviations, MemoryStack &scratch)
      {
        // halfWidth = ceil(num_std*sigma);
        const s32 halfWidth = 1 + ((sigma*numStandardDeviations) >> numSigmaFractionalBits);
        const f32 halfWidthF32 = static_cast<f32>(halfWidth);

        FixedPointArray<Type> gaussianKernel(1, 2*halfWidth + 1, numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false));
        Type * restrict pGaussianKernel = gaussianKernel.Pointer(0,0);

        {
          PUSH_MEMORY_STACK(scratch);

          Array<f32> gaussianKernelF32(1, 2*halfWidth + 1, scratch);
          f32 * restrict pGaussianKernelF32 = gaussianKernelF32.Pointer(0,0);

          const f32 twoTimesSigmaSquared = static_cast<f32>(2*sigma*sigma) / powf(2.0f, static_cast<f32>(numSigmaFractionalBits*2));

          s32 i = 0;
          f32 sum = 0;
          for(f32 x=-halfWidthF32; i<(2*halfWidth+1); x++, i++) {
            const f32 g = expf(-(x*x) / twoTimesSigmaSquared);
            pGaussianKernelF32[i] = g;
            sum += g;
          }

          // Normalize to sum to one
          const f32 sumInverse = 1.0f / sum;
          const f32 twoToNumBits = powf(2.0f, static_cast<f32>(numSigmaFractionalBits));
          for(s32 i=0; i<(2*halfWidth+1); i++) {
            const f32 gScaled = pGaussianKernelF32[i] * sumInverse * twoToNumBits;
            pGaussianKernel[i] = saturate_cast<Type>(gScaled);
          }

          // gaussianKernelF32.Print("gaussianKernelF32");
        } // PUSH_MEMORY_STACK(scratch);

        // gaussianKernel.Print("gaussianKernel");

        return gaussianKernel;
      } // Get1dGaussianKernel

      // NOTE: uses a 32-bit accumulator, so be careful of overflows
      template<typename InType, typename IntermediateType, typename OutType> NO_INLINE Result Correlate1d(const FixedPointArray<InType> &in1, const FixedPointArray<InType> &in2, FixedPointArray<OutType> &out)
      {
        const s32 outputLength = in1.get_size(1) + in2.get_size(1) - 1;

        AnkiConditionalErrorAndReturnValue(AreValid(in1, in2, out),
          RESULT_FAIL_INVALID_OBJECT, "Correlate1d", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(in1.get_size(0) == 1 && in2.get_size(0) == 1 && out.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "Correlate1d", "Arrays must be 1d and horizontal");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == outputLength,
          RESULT_FAIL_INVALID_SIZE, "Correlate1d", "Out must be the size of in1 + in2 - 1");

        AnkiConditionalErrorAndReturnValue(NotAliased(in1, in2, out),
          RESULT_FAIL_ALIASED_MEMORY, "Correlate1d", "in1, in2, and out must be in different memory locations");

        const InType * restrict pU;
        const InType * restrict pV;
        OutType * restrict pOut = out.Pointer(0,0);

        // To simplify things, u is the longer of the two,
        // and v is the shorter of the two
        s32 uLength;
        s32 vLength;
        if(in1.get_size(1) > in2.get_size(1)) {
          pU = in1.Pointer(0,0);
          pV = in2.Pointer(0,0);
          uLength = in1.get_size(1);
          vLength = in2.get_size(1);
        } else {
          pU = in2.Pointer(0,0);
          pV = in1.Pointer(0,0);
          uLength = in2.get_size(1);
          vLength = in1.get_size(1);
        }

        const s32 midStartIndex = vLength - 1;
        const s32 midEndIndex = midStartIndex + uLength - vLength;

        s32 shiftMagnitude;
        bool shiftRight;
        GetBitShiftDirectionAndMagnitude(in1.get_numFractionalBits(), in2.get_numFractionalBits(), out.get_numFractionalBits(), shiftMagnitude, shiftRight);

        s32 iOut = 0;

        // Filter the left part
        for(s32 x=0; x<midStartIndex; x++) {
          IntermediateType sum = 0;
          for(s32 xx=0; xx<=x; xx++) {
            const IntermediateType toAdd = static_cast<IntermediateType>(pU[xx] * pV[vLength-x+xx-1]);
            sum += toAdd;
          }

          if(shiftRight) {
            sum >>= shiftMagnitude;
          } else {
            sum <<= shiftMagnitude;
          }

          pOut[iOut++] = static_cast<OutType>(sum);
        }

        // Filter the middle part
        for(s32 x=midStartIndex; x<=midEndIndex; x++) {
          IntermediateType sum = 0;
          for(s32 xx=0; xx<vLength; xx++) {
            const IntermediateType toAdd = static_cast<IntermediateType>(pU[x+xx-midStartIndex] * pV[xx]);
            sum += toAdd;
          }

          if(shiftRight) {
            sum >>= shiftMagnitude;
          } else {
            sum <<= shiftMagnitude;
          }

          pOut[iOut++] = static_cast<OutType>(sum);
        }

        // Filter the right part
        for(s32 x=midEndIndex+1; x<outputLength; x++) {
          const s32 vEnd = outputLength - x;
          IntermediateType sum = 0;
          for(s32 xx=0; xx<vEnd; xx++) {
            const IntermediateType toAdd = static_cast<IntermediateType>(pU[x+xx-midStartIndex] * pV[xx]);
            sum += toAdd;
          }

          if(shiftRight) {
            sum >>= shiftMagnitude;
          } else {
            sum <<= shiftMagnitude;
          }

          pOut[iOut++] = static_cast<OutType>(sum);
        }

        return RESULT_OK;
      } // Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

      template<typename InType, typename IntermediateType, typename OutType> NO_INLINE Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<InType> &image, const FixedPointArray<InType> &filter, FixedPointArray<OutType> &out, MemoryStack scratch)
      {
        BeginBenchmark("Correlate1dCircularAndSameSizeOutput");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        const s32 filterHeight = filter.get_size(0);
        const s32 filterWidth = filter.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(image, filter, out),
          RESULT_FAIL_INVALID_OBJECT, "Correlate1dCircularAndSameSizeOutput", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(imageHeight==1 && filterHeight==1 && out.get_size(0)==1,
          RESULT_FAIL_INVALID_SIZE, "Correlate1dCircularAndSameSizeOutput", "Arrays must be 1d and horizontal");

        AnkiConditionalErrorAndReturnValue(imageWidth > filterWidth,
          RESULT_FAIL_INVALID_SIZE, "Correlate1dCircularAndSameSizeOutput", "The image must be larger than the filter");

        AnkiConditionalErrorAndReturnValue(NotAliased(image, filter, out),
          RESULT_FAIL_ALIASED_MEMORY, "Correlate1dCircularAndSameSizeOutput", "in1, in2, and out must be in different memory locations");

        Array<InType> paddedImage(1, imageWidth + 2*(filterWidth-1), scratch);

        paddedImage(0,0,0,filterWidth-2).Set(image(0,0,-filterWidth+1,-1));
        paddedImage(0,0,filterWidth-1,filterWidth+imageWidth-2).Set(image);
        paddedImage(0,0,filterWidth+imageWidth-1,-1).Set(image(0,0,0,filterWidth-2));

        //const InType * restrict pImage = image.Pointer(0,0);

        //image.Print("image");
        //paddedImage.Print("paddedImage");

        //Matlab matlab(false);
        //matlab.PutArray(image, "image");
        //matlab.PutArray(paddedImage, "paddedImage");

        const InType * restrict pPaddedImage = paddedImage.Pointer(0,0);
        const InType * restrict pFilter = filter.Pointer(0,0);
        OutType * restrict pOut = out.Pointer(0,0);

        s32 shiftMagnitude;
        bool shiftRight;
        GetBitShiftDirectionAndMagnitude(image.get_numFractionalBits(), filter.get_numFractionalBits(), out.get_numFractionalBits(), shiftMagnitude, shiftRight);

        s32 shiftType = !!shiftMagnitude;
        if(shiftRight)
          shiftType = 2;

        const s32 filterHalfWidth = filterWidth >> 1;

        for(s32 x=0; x<imageWidth; x++) {
          IntermediateType sum = 0;

          if(sizeof(InType) == 4 && Flags::TypeCharacteristics<InType>::isSigned) {
            const s32 filterWidthSimdMax = RoundDown<s32>(filterWidth, 2);
            for(s32 xFilter=0; xFilter<filterWidthSimdMax; xFilter+=2) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;

              const IntermediateType toAdd0 = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              const IntermediateType toAdd1 = static_cast<IntermediateType>(pPaddedImage[xImage+1] * pFilter[xFilter+1]);

              sum += toAdd0 + toAdd1;
            }

            for(s32 xFilter=filterWidthSimdMax; xFilter<filterWidth; xFilter++) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;
              const IntermediateType toAdd = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              sum += toAdd;
            }
          } else if(sizeof(InType) == 2 && Flags::TypeCharacteristics<InType>::isSigned) {
            //const s32 xImageOffset = x - filterHalfWidth + filterWidth - 1;

            s32 xImage = x - filterHalfWidth + filterWidth - 1;

            const s32 filterWidthSimdMax = RoundDown<s32>(filterWidth, 8);
            for(s32 xFilter=0; xFilter<filterWidthSimdMax; xFilter+=8, xImage+=8) {
              //#if !defined(USE_ARM_ACCELERATION_IMAGE_PROCESSING)
              const IntermediateType toAdd0 = static_cast<IntermediateType>(pPaddedImage[xImage]   * pFilter[xFilter]);
              const IntermediateType toAdd1 = static_cast<IntermediateType>(pPaddedImage[xImage+1] * pFilter[xFilter+1]);
              const IntermediateType toAdd2 = static_cast<IntermediateType>(pPaddedImage[xImage+2] * pFilter[xFilter+2]);
              const IntermediateType toAdd3 = static_cast<IntermediateType>(pPaddedImage[xImage+3] * pFilter[xFilter+3]);
              const IntermediateType toAdd4 = static_cast<IntermediateType>(pPaddedImage[xImage+4] * pFilter[xFilter+4]);
              const IntermediateType toAdd5 = static_cast<IntermediateType>(pPaddedImage[xImage+5] * pFilter[xFilter+5]);
              const IntermediateType toAdd6 = static_cast<IntermediateType>(pPaddedImage[xImage+6] * pFilter[xFilter+6]);
              const IntermediateType toAdd7 = static_cast<IntermediateType>(pPaddedImage[xImage+7] * pFilter[xFilter+7]);

              sum += toAdd0 + toAdd1 + toAdd2 + toAdd3 + toAdd4 + toAdd5 + toAdd6 + toAdd7;
              //#else // #if !defined(USE_ARM_ACCELERATION_IMAGE_PROCESSING)
              //              // TODO: make work for any template type, and so Keil doesn't merge loads
              //              const u32 image01 = *reinterpret_cast<const u32*>(&pPaddedImage[xImage]);
              //              const u32 image23 = *reinterpret_cast<const u32*>(&pPaddedImage[xImage + 2]);
              //              const u32 image45 = *reinterpret_cast<const u32*>(&pPaddedImage[xImage + 4]);
              //              const u32 image67 = *reinterpret_cast<const u32*>(&pPaddedImage[xImage + 6]);
              //
              //              const u32 filter01 = *reinterpret_cast<const u32*>(&pFilter[xFilter]);
              //              const u32 filter23 = *reinterpret_cast<const u32*>(&pFilter[xFilter + 2]);
              //              const u32 filter45 = *reinterpret_cast<const u32*>(&pFilter[xFilter + 4]);
              //              const u32 filter67 = *reinterpret_cast<const u32*>(&pFilter[xFilter + 6]);
              //
              //              sum = __SMLAD(image01, filter01, sum);
              //              sum = __SMLAD(image23, filter23, sum);
              //              sum = __SMLAD(image45, filter45, sum);
              //              sum = __SMLAD(image67, filter67, sum);
              //#endif // #if !defined(USE_ARM_ACCELERATION_IMAGE_PROCESSING) ... #else
            }

            for(s32 xFilter=filterWidthSimdMax; xFilter<filterWidth; xFilter++) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;
              const IntermediateType toAdd = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              sum += toAdd;
            }
          } else {
            for(s32 xFilter=0; xFilter<filterWidth; xFilter++) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;
              const IntermediateType toAdd = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              sum += toAdd;
            }
          }

          if(shiftType == 2) {
            sum >>= shiftMagnitude;
          } else if(shiftType == 1) {
            sum <<= shiftMagnitude;
          }

          pOut[x] = static_cast<OutType>(sum);
        } // for(s32 x=0; x<imageWidth; x++)

        EndBenchmark("Correlate1dCircularAndSameSizeOutput");

        //CoreTechPrint("numLeft:%d numRight:%d numCenter:%d\n", numLeft, numRight, numCenter);

        return RESULT_OK;
      } // Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

      template<typename Type> Result LocalMaxima(const Array<Type> &in, const s32 searchHeight, const s32 searchWidth, FixedLengthList<Point<s16> > &points)
      {
        AnkiConditionalErrorAndReturnValue(AreValid(in, points),
          RESULT_FAIL_INVALID_OBJECT, "LocalMaxima", "Invalid inputs");

        // TODO: support other sizes
        AnkiConditionalErrorAndReturnValue(searchHeight == 3 && searchWidth == 3,
          RESULT_FAIL_INVALID_PARAMETER, "LocalMaxima", "Currently only 3x3 supported");

        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        for(s32 y=1; y<(inHeight-1); y++) {
          const Type * restrict pIn_ym1 = in.Pointer(y-1,0);
          const Type * restrict pIn_y0 = in.Pointer(y,0);
          const Type * restrict pIn_yp1 = in.Pointer(y+1,0);

          for(s32 x=1; x<(inWidth-1); x++) {
            const Type centerValue = pIn_y0[x];

            if(centerValue > pIn_ym1[x-1] && centerValue > pIn_ym1[x] && centerValue > pIn_ym1[x+1] &&
              centerValue > pIn_y0[x-1]   &&                             centerValue > pIn_y0[x+1] &&
              centerValue > pIn_yp1[x-1]  && centerValue > pIn_yp1[x] && centerValue > pIn_yp1[x+1])
            {
              points.PushBack(Point<s16>(x,y));
            }
          }
        }

        return RESULT_OK;
      } // LocalMaxima
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
