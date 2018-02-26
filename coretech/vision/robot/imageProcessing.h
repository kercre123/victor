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

#include "coretech/vision/robot/imageProcessing_declarations.h"
#include "coretech/common/robot/interpolate.h"

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/comparisons.h"

//#include "coretech/common/robot/matlabInterface.h"

#include "coretech/common/robot/hostIntrinsics_m4.h"

#define ACCELERATION_NONE 0
#define ACCELERATION_ARM_M4 1
#define ACCELERATION_ARM_A7 2

#ifdef __ARM_NEON__
#define ACCELERATION_TYPE ACCELERATION_ARM_A7
#else
#define ACCELERATION_TYPE ACCELERATION_NONE
#endif

#if ACCELERATION_TYPE == ACCELERATION_ARM_A7
#include <arm_neon.h>
#endif


template<u8 upsamplePowerU8> void UpsampleByPowerOfTwoBilinear_innerLoop(
  const u8 * restrict pInY0,
  const u8 * restrict pInY1,
  Anki::Embedded::Array<u8> &out,
  const s32 ySmall,
  const s32 smallWidth,
  const s32 outStride);

template<u8 upsamplePowerU8> void UpsampleByPowerOfTwoBilinear_innerLoop(
  const u8 * restrict pInY0,
  const u8 * restrict pInY1,
  Anki::Embedded::Array<u8> &out,
  const s32 ySmall,
  const s32 smallWidth,
  const s32 outStride)
{
  const u8 upsampleFactorU8 = 1 << upsamplePowerU8;

  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++) {
    const u8 smallUL = pInY0[xSmall];
    const u8 smallUR = pInY0[xSmall+1];
    const u8 smallLL = pInY1[xSmall];
    const u8 smallLR = pInY1[xSmall+1];

    u8 * restrict pOut = out.Pointer(ySmall*upsampleFactorU8 + upsampleFactorU8/2, 0);

    const s32 xBig0 = xSmall*upsampleFactorU8 + upsampleFactorU8/2;

    for(s32 dy=0; dy<upsampleFactorU8; dy++) {
      const u8 alpha = (u8)(2*upsampleFactorU8 - 2*dy - 1);
      const u8 alphaInverse = (u8)(2*dy + 1);

      const u16 interpolatedPixelL0 = smallUL * alpha;
      const u16 interpolatedPixelL1 = smallLL * alphaInverse;
      const u16 interpolatedPixelL = interpolatedPixelL0 + interpolatedPixelL1;
      const u16 subtractAmount = interpolatedPixelL >> (upsamplePowerU8-1);

      const u16 interpolatedPixelR0 = smallUR * alpha;
      const u16 interpolatedPixelR1 = smallLR * alphaInverse;
      const u16 interpolatedPixelR = interpolatedPixelR0 + interpolatedPixelR1;
      const u16 addAmount = interpolatedPixelR >> (upsamplePowerU8-1);

      u16 curValue = 2*interpolatedPixelL + ((addAmount - subtractAmount)>>1);

      for(s32 dx=0; dx<upsampleFactorU8; dx++) {
        const u8 curValueU8 = (u8)(curValue >> (upsamplePowerU8+2));

        pOut[xBig0 + dx] = curValueU8;

        curValue += addAmount - subtractAmount;
      } // for(s32 dx=0; dx<upsampleFactorU8; dx++)

      pOut += outStride;
    } // for(s32 dy=0; dy<upsampleFactorU8; dy++)
  } //  for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++)
} // UpsampleByPowerOfTwoBilinear_innerLoop()

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
        const InType kernel0 = 1;
        const InType kernel1 = 4;
        const InType kernel2 = 6;
        const InType kernel3 = 4;
        const InType kernel4 = 1;

        const s32 kernelShift = 4;

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(image, imageFiltered, scratch),
          RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(imageHeight == imageFiltered.get_size(0) && imageWidth == imageFiltered.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "BinomialFilter", "size(image) != size(imageFiltered) (%dx%d != %dx%d)", imageHeight, imageWidth, imageHeight, imageWidth);

        AnkiConditionalErrorAndReturnValue(NotAliased(image, imageFiltered),
          RESULT_FAIL_ALIASED_MEMORY, "BinomialFilter", "image and imageFiltered must be different");

        Array<IntermediateType> imageFilteredTmp(imageHeight, imageWidth, scratch);

        AnkiAssert(imageFilteredTmp.get_stride() % sizeof(IntermediateType) == 0);

        const s32 imageFilteredTmpStep = imageFilteredTmp.get_stride() / sizeof(IntermediateType);

        //% 1. Horizontally filter
        for(s32 y=0; y<imageHeight; y++) {
          //
          // First, filter horizontally
          //

          const InType * restrict pImage = image.Pointer(y, 0);
          IntermediateType * restrict pImageFilteredTmp = imageFilteredTmp.Pointer(y, 0);

          s32 x = 0;

          pImageFilteredTmp[x] = static_cast<IntermediateType>( (pImage[x]*kernel2 + pImage[x+1]*kernel3 + pImage[x+2]*kernel4 + pImage[x]*(kernel0+kernel1)) >> kernelShift );
          x++;
          pImageFilteredTmp[x] = static_cast<IntermediateType>( (pImage[x-1]*kernel1 + pImage[x]*kernel2   + pImage[x+1]*kernel3 + pImage[x+2]*kernel4 + pImage[x-1]*kernel0) >> kernelShift );
          x++;

          for(; x<(imageWidth-2); x++) {
            pImageFilteredTmp[x] = static_cast<IntermediateType>( (pImage[x-2]*kernel0 + pImage[x-1]*kernel1 + pImage[x]*kernel2 + pImage[x+1]*kernel3 + pImage[x+2]*kernel4) >> kernelShift );
          }

          pImageFilteredTmp[x] = static_cast<IntermediateType>( (pImage[x-2]*kernel0 + pImage[x-1]*kernel1 + pImage[x]*kernel2 + pImage[x+1]*kernel3 + pImage[x+1]*kernel4) >> kernelShift );
          x++;
          pImageFilteredTmp[x] = static_cast<IntermediateType>( (pImage[x-2]*kernel0 + pImage[x-1]*kernel1 + pImage[x]*kernel2 + pImage[x]*(kernel3+kernel4)) >> kernelShift );
          x++;

          //
          // At a delayed line, filter vertically
          //

          if(y > 1) {
            const IntermediateType * restrict pImageFilteredTmpYm2 = pImageFilteredTmp - 4*imageFilteredTmpStep;
            const IntermediateType * restrict pImageFilteredTmpYm1 = pImageFilteredTmp - 3*imageFilteredTmpStep;
            const IntermediateType * restrict pImageFilteredTmpY0  = pImageFilteredTmp - 2*imageFilteredTmpStep;
            const IntermediateType * restrict pImageFilteredTmpYp1 = pImageFilteredTmp -   imageFilteredTmpStep;
            const IntermediateType * restrict pImageFilteredTmpYp2 = pImageFilteredTmp;

            OutType * restrict pImageFiltered = imageFiltered.Pointer(y-2, 0);

            if(y == 2) {
              for(s32 x=0; x<imageWidth; x++) {
                pImageFiltered[x] = static_cast<OutType>( (pImageFilteredTmpY0[x]*(kernel0+kernel1+kernel2) + pImageFilteredTmpYp1[x]*kernel3 + pImageFilteredTmpYp2[x]*kernel4) >> kernelShift);
              }
            } else if(y == 3) {
              for(s32 x=0; x<imageWidth; x++) {
                pImageFiltered[x] = static_cast<OutType>( (pImageFilteredTmpYm1[x]*(kernel0+kernel1) + pImageFilteredTmpY0[x]*kernel2 + pImageFilteredTmpYp1[x]*kernel3 + pImageFilteredTmpYp2[x]*kernel4) >> kernelShift);
              }
            } else { // y >= 4
              for(s32 x=0; x<imageWidth; x++) {
                pImageFiltered[x] = static_cast<OutType>( (pImageFilteredTmpYm2[x]*kernel0 + pImageFilteredTmpYm1[x]*kernel1 + pImageFilteredTmpY0[x]*kernel2 + pImageFilteredTmpYp1[x]*kernel3 + pImageFilteredTmpYp2[x]*kernel4) >> kernelShift);
              }
            }
          } // if(y > 2)
        } // for(s32 y=0; y<imageHeight; y++)

        // Do final two rows

        const IntermediateType * restrict pImageFilteredTmpYEndm4 = imageFilteredTmp.Pointer(imageFilteredTmp.get_size(0)-4, 0);;
        const IntermediateType * restrict pImageFilteredTmpYEndm3 = pImageFilteredTmpYEndm4 + imageFilteredTmpStep;
        const IntermediateType * restrict pImageFilteredTmpYEndm2 = pImageFilteredTmpYEndm4 + 2*imageFilteredTmpStep;
        const IntermediateType * restrict pImageFilteredTmpYEndm1 = pImageFilteredTmpYEndm4 + 3*imageFilteredTmpStep;

        OutType * restrict pImageFilteredYEndm2 = imageFiltered.Pointer(imageFiltered.get_size(0)-2, 0);
        OutType * restrict pImageFilteredYEndm1 = imageFiltered.Pointer(imageFiltered.get_size(0)-1, 0);

        for(s32 x=0; x<imageWidth; x++) {
          pImageFilteredYEndm2[x] = static_cast<OutType>( (pImageFilteredTmpYEndm4[x]*kernel0 + pImageFilteredTmpYEndm3[x]*kernel1 + pImageFilteredTmpYEndm2[x]*kernel2 + pImageFilteredTmpYEndm1[x]*(kernel3+kernel4))         >> kernelShift);
          pImageFilteredYEndm1[x] = static_cast<OutType>( (                                     pImageFilteredTmpYEndm3[x]*kernel0 + pImageFilteredTmpYEndm2[x]*kernel1 + pImageFilteredTmpYEndm1[x]*(kernel2+kernel3+kernel4)) >> kernelShift);
        }

        return RESULT_OK;
      } // BinomialFilter()

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
        AnkiConditionalErrorAndReturnValue(AreValid(image, filtered),
          RESULT_FAIL_INVALID_OBJECT, "BoxFilter", "Image is invalid");

        AnkiConditionalErrorAndReturnValue(NotAliased(image, filtered),
          RESULT_FAIL_ALIASED_MEMORY, "BoxFilter", "Images are aliased");

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

      template<typename InType, typename IntermediateType, typename OutType> FixedLengthList<Array<OutType> > BuildPyramid(
        const Array<InType> &image, //< WARNING: the memory for "image" is used by the first level of the pyramid.
        const s32 numPyramidLevels, //< The number of levels in the pyramid is numPyramidLevels + 1, so can be 0 for a single image. The image size must be evenly divisible by "2^numPyramidLevels".
        MemoryStack &memory)  //< Memory for the output will be allocated by this function
      {
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(image.IsValid() && numPyramidLevels >= 0,
          FixedLengthList<Array<OutType> >(), "BuildPyramid", "Invalid inputs");

        for(s32 iLevel=1; iLevel<numPyramidLevels; iLevel++) {
          const s32 scaledHeight = imageHeight >> iLevel;
          const s32 scaledWidth = imageWidth >> iLevel;

          AnkiConditionalErrorAndReturnValue(scaledHeight%2 == 0 && scaledWidth%2 == 0,
            FixedLengthList<Array<OutType> >(), "BuildPyramid", "Too many pyramid levels requested");
        }

        FixedLengthList<Array<OutType> > pyramid(numPyramidLevels + 1, memory);

        AnkiConditionalErrorAndReturnValue(pyramid.IsValid(),
          FixedLengthList<Array<OutType> >(), "BuildPyramid", "Out of memory");

        pyramid.set_size(numPyramidLevels+1);

        pyramid[0] = image;

        for(s32 iLevel=1; iLevel<=numPyramidLevels; iLevel++) {
          const s32 scaledHeight = imageHeight >> iLevel;
          const s32 scaledWidth = imageWidth >> iLevel;
          pyramid[iLevel] = Array<OutType>(scaledHeight, scaledWidth, memory);

          AnkiConditionalErrorAndReturnValue(pyramid[iLevel].IsValid(),
            FixedLengthList<Array<OutType> >(), "BuildPyramid", "Out of memory");

          /*const Result result = */ImageProcessing::DownsampleByTwo<InType,IntermediateType,OutType>(pyramid[iLevel-1], pyramid[iLevel]);
        } // for(s32 iLevel=1; iLevel<=numPyramidLevels; iLevel++)

        return pyramid;
      } // BuildPyramid()

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

#if ACCELERATION_TYPE == ACCELERATION_NONE || ACCELERATION_TYPE == ACCELERATION_ARM_A7
        for(s32 x=0; x<imageWidth; x++) {
          IntermediateType sum = 0;

          for(s32 xFilter=0; xFilter<filterWidth; xFilter++) {
            const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;
            const IntermediateType toAdd = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
            sum += toAdd;
          }

          if(shiftType == 2) {
            sum >>= shiftMagnitude;
          } else if(shiftType == 1) {
            sum <<= shiftMagnitude;
          }

          pOut[x] = static_cast<OutType>(sum);
        } // for(s32 x=0; x<imageWidth; x++)
#else // #if ACCELERATION_TYPE == ACCELERATION_NONE || ACCELERATION_TYPE == ACCELERATION_ARM_A7
        for(s32 x=0; x<imageWidth; x++) {
          IntermediateType sum = 0;

          if(sizeof(InType) == 4 && Flags::TypeCharacteristics<InType>::isSigned) {
            s32 xFilter = 0;

            const s32 filterWidthSimdMax = RoundDown<s32>(filterWidth, 2);
            for(; xFilter<filterWidthSimdMax; xFilter+=2) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;

              const IntermediateType toAdd0 = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              const IntermediateType toAdd1 = static_cast<IntermediateType>(pPaddedImage[xImage+1] * pFilter[xFilter+1]);

              sum += toAdd0 + toAdd1;
            }

            for(; xFilter<filterWidth; xFilter++) {
              const s32 xImage = x - filterHalfWidth + filterWidth - 1 + xFilter;
              const IntermediateType toAdd = static_cast<IntermediateType>(pPaddedImage[xImage] * pFilter[xFilter]);
              sum += toAdd;
            }
          } else if(sizeof(InType) == 2 && Flags::TypeCharacteristics<InType>::isSigned) {
            //const s32 xImageOffset = x - filterHalfWidth + filterWidth - 1;

            s32 xFilter = 0;

            s32 xImage = x - filterHalfWidth + filterWidth - 1;

            const s32 filterWidthSimdMax = RoundDown<s32>(filterWidth, 8);
            for(; xFilter<filterWidthSimdMax; xFilter+=8, xImage+=8) {
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

            for(; xFilter<filterWidth; xFilter++) {
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
#endif // #if ACCELERATION_TYPE == ACCELERATION_NONE ... #else

        EndBenchmark("Correlate1dCircularAndSameSizeOutput");

        //CoreTechPrint("numLeft:%d numRight:%d numCenter:%d\n", numLeft, numRight, numCenter);

        return RESULT_OK;
      } // Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

      template<typename Type> Result LocalMaxima(const Array<Type> &in, const s32 searchHeight, const s32 searchWidth, FixedLengthList<Point<s16> > &points, FixedLengthList<Type> *values)
      {
        AnkiConditionalErrorAndReturnValue(AreValid(in, points),
          RESULT_FAIL_INVALID_OBJECT, "LocalMaxima", "Invalid inputs");

        points.Clear();

        if(values) {
          AnkiConditionalErrorAndReturnValue(values->IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LocalMaxima", "Invalid inputs");

          values->Clear();
        }

        // TODO: support other sizes
        AnkiConditionalErrorAndReturnValue(searchHeight == 3 && searchWidth == 3,
          RESULT_FAIL_INVALID_PARAMETER, "LocalMaxima", "Currently only 3x3 supported");

        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        for(s32 y=1; y<(inHeight-1); y++) {
          const Type * restrict pIn_ym1 = in.Pointer(y-1,0);
          const Type * restrict pIn_y0 = in.Pointer(y,0);
          const Type * restrict pIn_yp1 = in.Pointer(y+1,0);

          if(values) {
            for(s32 x=1; x<(inWidth-1); x++) {
              const Type centerValue = pIn_y0[x];

              if(centerValue > pIn_ym1[x-1] && centerValue > pIn_ym1[x] && centerValue > pIn_ym1[x+1] &&
                centerValue > pIn_y0[x-1]   &&                             centerValue > pIn_y0[x+1] &&
                centerValue > pIn_yp1[x-1]  && centerValue > pIn_yp1[x] && centerValue > pIn_yp1[x+1])
              {
                points.PushBack(Point<s16>(x,y));
                values->PushBack(centerValue);
              }
            }
          } else { // if(values)
            for(s32 x=1; x<(inWidth-1); x++) {
              const Type centerValue = pIn_y0[x];

              if(centerValue > pIn_ym1[x-1] && centerValue > pIn_ym1[x] && centerValue > pIn_ym1[x+1] &&
                centerValue > pIn_y0[x-1]   &&                             centerValue > pIn_y0[x+1] &&
                centerValue > pIn_yp1[x-1]  && centerValue > pIn_yp1[x] && centerValue > pIn_yp1[x+1])
              {
                points.PushBack(Point<s16>(x,y));
              }
            }
          } // if(values) ... else
        }

        return RESULT_OK;
      } // LocalMaxima

      template<int upsamplePower> NO_INLINE Result UpsampleByPowerOfTwoBilinear(const Array<u8> &in, Array<u8> &out, MemoryStack scratch)
      {
        // The correct weights would be given by d2, though we approximate them in this function
        // dSize=2; ds=zeros(dSize,dSize,4); ds(1,1,1)=1; ds(1,end,2)=1; ds(end,1,3)=1; ds(end,end,4)=1;
        // d2 = imresize(ds, [size(ds,1), size(ds,2)]*upsampleFactor, 'bilinear', 'Antialiasing', false)

        const s32 largeHeight = out.get_size(0);
        const s32 largeWidth = out.get_size(1);

        const s32 smallHeight = largeHeight >> upsamplePower;
        const s32 smallWidth = largeWidth >> upsamplePower;

        const s32 outStride = out.get_stride();

        AnkiConditionalErrorAndReturnValue(AreValid(in, out , scratch),
          RESULT_FAIL_INVALID_OBJECT, "UpsampleByPowerOfTwoBilinear", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(smallHeight, smallWidth, in),
          RESULT_FAIL_INVALID_SIZE, "UpsampleByPowerOfTwoBilinear", "size(out) is not equal to size(in) << downsampleFactor");

        AnkiConditionalErrorAndReturnValue(largeWidth % 4 == 0,
          RESULT_FAIL_INVALID_SIZE, "UpsampleByPowerOfTwoBilinear", "The width of the in Array must be a multiple of four");

        AnkiConditionalErrorAndReturnValue(upsamplePower > 0 && upsamplePower < 8,
          RESULT_FAIL_INVALID_PARAMETER, "UpsampleByPowerOfTwoBilinear", "0 < upsamplePower < 8");

        const u8 upsamplePowerU8 = upsamplePower;
        const u8 upsampleFactorU8 = 1 << upsamplePowerU8;

        // The correct weights would be given by d2, though we approximate them in this function
        // dSize=2; ds=zeros(dSize,dSize,4); ds(1,1,1)=1; ds(1,end,2)=1; ds(end,1,3)=1; ds(end,end,4)=1;
        // d2 = imresize(ds, [size(ds,1), size(ds,2)]*4, 'bilinear', 'Antialiasing', false)

        // TODO: do the edges

        // Just compute the boxWidth*UL in an accumulator, and as you go right, subtract and add. As you go down, subtract and add top vs bottom

        // const s32 ySmall = -1;
        {
          const s32 ySmall = -1;

          const u8 * restrict pInY0 = in.Pointer(0, 0);

          out(0, (upsampleFactorU8>>1)-1, 0, (upsampleFactorU8>>1)-1).Set(pInY0[0]);

          for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++) {
            const u8 smallL = pInY0[xSmall];
            const u8 smallR = pInY0[xSmall+1];

            for(s32 dy=upsampleFactorU8>>1; dy<upsampleFactorU8; dy++) {
              u8 * restrict pOut = out.Pointer(ySmall*upsampleFactorU8 + upsampleFactorU8/2 + dy, 0);

              const u16 subtractAmount = (u16)(smallL << 2);
              const u16 addAmount = (u16)(smallR << 2);

              const s32 xBig0 = xSmall*upsampleFactorU8 + upsampleFactorU8/2;

              u16 curValue = (u16)((smallL << (upsamplePowerU8+2)) + ((addAmount - subtractAmount)>>1));

              for(s32 dx=0; dx<upsampleFactorU8; dx++) {
                const u8 curValueU8 = (u8)(curValue >> (upsamplePowerU8+2));

                pOut[xBig0 + dx] = curValueU8;

                curValue += addAmount - subtractAmount;
              } // for(s32 dx=0; dx<upsampleFactorU8; dx++)
            } // for(s32 dy=0; dy<upsampleFactorU8; dy++)
          } // for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++)

          out(0, (upsampleFactorU8>>1)-1, -(upsampleFactorU8>>1), -1).Set(pInY0[smallWidth-1]);
        } // const s32 ySmall = -1;

        for(s32 ySmall=0; ySmall<smallHeight-1; ySmall++) {
          const u8 * restrict pInY0 = in.Pointer(ySmall, 0);
          const u8 * restrict pInY1 = in.Pointer(ySmall+1, 0);

          // const s32 xSmall = -1;
          {
            const s32 xSmall = -1;

            const u8 smallU = pInY0[0];
            const u8 smallL = pInY1[0];

            for(s32 dy=0; dy<upsampleFactorU8; dy++) {
              u8 * restrict pOut = out.Pointer(ySmall*upsampleFactorU8 + upsampleFactorU8/2 + dy, 0);

              const u8 alpha = (u8)(2*upsampleFactorU8 - 2*dy - 1);
              const u8 alphaInverse = (u8)(2*dy + 1);

              const u16 interpolatedPixelL0 = smallU * alpha;
              const u16 interpolatedPixelL1 = smallL * alphaInverse;
              const u16 interpolatedPixelL = interpolatedPixelL0 + interpolatedPixelL1;
              const u8 curValueU8 = (u8)(interpolatedPixelL >> (upsamplePowerU8+1));

              const s32 xBig0 = xSmall*upsampleFactorU8 + upsampleFactorU8/2;

              for(s32 dx=upsampleFactorU8>>1; dx<upsampleFactorU8; dx++) {
                pOut[xBig0 + dx] = curValueU8;
              } // for(s32 dx=0; dx<upsampleFactorU8; dx++)
            } // for(s32 dy=0; dy<upsampleFactorU8; dy++)
          } // const s32 xSmall = -1;

          UpsampleByPowerOfTwoBilinear_innerLoop<upsamplePower>(pInY0, pInY1, out, ySmall, smallWidth, outStride);

          // const s32 xSmall = smallWidth-1;
          {
            const s32 xSmall = smallWidth-1;

            const u8 smallU = pInY0[smallWidth-1];
            const u8 smallL = pInY1[smallWidth-1];

            for(s32 dy=0; dy<upsampleFactorU8; dy++) {
              u8 * restrict pOut = out.Pointer(ySmall*upsampleFactorU8 + upsampleFactorU8/2 + dy, 0);

              const u8 alpha = (u8)(2*upsampleFactorU8 - 2*dy - 1);
              const u8 alphaInverse = (u8)(2*dy + 1);

              const u16 interpolatedPixelL0 = smallU * alpha;
              const u16 interpolatedPixelL1 = smallL * alphaInverse;
              const u16 interpolatedPixelL = interpolatedPixelL0 + interpolatedPixelL1;
              const u8 curValueU8 = (u8)(interpolatedPixelL >> (upsamplePowerU8+1));

              const s32 xBig0 = xSmall*upsampleFactorU8 + upsampleFactorU8/2;

              for(s32 dx=0; dx<upsampleFactorU8>>1; dx++) {
                pOut[xBig0 + dx] = curValueU8;
              } // for(s32 dx=0; dx<upsampleFactorU8; dx++)
            } // for(s32 dy=0; dy<upsampleFactorU8; dy++)
          } // const s32 xSmall = smallWidth-1;
        } //  for(s32 ySmall=0; ySmall<smallHeight-1; ySmall++)

        // const s32 ySmall = smallHeight - 1;
        {
          const s32 ySmall = smallHeight - 1;

          const u8 * restrict pInY0 = in.Pointer(smallHeight - 1, 0);

          out(-(upsampleFactorU8>>1), -1, 0, (upsampleFactorU8>>1)-1).Set(pInY0[0]);

          for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++) {
            const u8 smallL = pInY0[xSmall];
            const u8 smallR = pInY0[xSmall+1];

            for(s32 dy=0; dy<upsampleFactorU8>>1; dy++) {
              u8 * restrict pOut = out.Pointer(ySmall*upsampleFactorU8 + upsampleFactorU8/2 + dy, 0);

              const u16 subtractAmount = (u16)(smallL << 2);
              const u16 addAmount = (u16)(smallR << 2);

              const s32 xBig0 = xSmall*upsampleFactorU8 + upsampleFactorU8/2;

              u16 curValue = (u16)((smallL << (upsamplePowerU8+2)) + ((addAmount - subtractAmount)>>1));

              for(s32 dx=0; dx<upsampleFactorU8; dx++) {
                const u8 curValueU8 = (u8)(curValue >> (upsamplePowerU8+2));

                pOut[xBig0 + dx] = curValueU8;

                curValue += addAmount - subtractAmount;
              } // for(s32 dx=0; dx<upsampleFactorU8; dx++)
            } // for(s32 dy=0; dy<upsampleFactorU8; dy++)
          } // for(s32 xSmall=0; xSmall<smallWidth-1; xSmall++)

          out(-(upsampleFactorU8>>1), -1, -(upsampleFactorU8>>1), -1).Set(pInY0[smallWidth-1]);
        } // const s32 ySmall = smallHeight - 1;

        return RESULT_OK;
      } // Result UpsampleBilinear(const Array<u8> &in, Array<u8> &out, MemoryStack scratch)
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
