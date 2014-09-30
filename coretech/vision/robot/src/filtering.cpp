/**
File: filtering.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"

#include "anki/common/constantsAndMacros.h"

#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/hostIntrinsics_m4.h"

#define USE_ARM_ACCELERATION

#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      void GetBitShiftDirectionAndMagnitude(
        const s32 in1_numFractionalBits,
        const s32 in2_numFractionalBit,
        const s32 out_numFractionalBit,
        s32 &shiftMagnitude,
        bool &shiftRight)
      {
        if((in1_numFractionalBits+in2_numFractionalBit) > out_numFractionalBit) {
          shiftRight = true;
          shiftMagnitude = in1_numFractionalBits + in2_numFractionalBit - out_numFractionalBit;
        } else if((in1_numFractionalBits+in2_numFractionalBit) < out_numFractionalBit) {
          shiftRight = false;
          shiftMagnitude = out_numFractionalBit - in1_numFractionalBits - in2_numFractionalBit;
        } else {
          shiftRight = false;
          shiftMagnitude = 0;
        }
      } // void GetBitShiftDirectionAndMagnitude()

      /*
      // This is a general function with specializations defined for relevant
      // types
      template<typename Type> Type GetImageTypeMean(void);

      // TODO: these should be in a different .cpp file (imageProcessing.cpp?)
      template<> f32 GetImageTypeMean<f32>() { return 0.5f; }
      template<> u8  GetImageTypeMean<u8>()  { return 128;  }
      template<> f64 GetImageTypeMean<f64>() { return 0.5f; }
      */

      Result BoxFilterNormalize(const Array<u8> &image, const s32 boxSize, const u8 padValue,
        Array<u8> &imageNorm, MemoryStack scratch)
      {
        Result lastResult = RESULT_OK;

        AnkiConditionalErrorAndReturnValue(image.IsValid(),
          RESULT_FAIL_INVALID_OBJECT,
          "BoxFilterNormalize",
          "Input image is invalid.");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth  = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(imageNorm.IsValid(),
          RESULT_FAIL_INVALID_OBJECT,
          "BoxFilterNormalize",
          "Output normalized image is invalid.");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, imageNorm),
          RESULT_FAIL_INVALID_SIZE,
          "BoxFilterNormalize",
          "Output normalized image must match input image's size.");

        Array<f32> integralImage(imageHeight, imageWidth, scratch);

        AnkiConditionalErrorAndReturnValue(integralImage.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY,
          "BoxFilterNormalize",
          "Could not allocate integral image (out of memory?).");

        if((lastResult = CreateIntegralImage(image, integralImage)) != RESULT_OK) {
          return lastResult;
        }

        // Divide each input pixel by box filter results computed from integral image
        const s32 halfWidth = MIN(MIN(imageWidth,imageHeight)-1, boxSize)/2;
        const s32 boxWidth  = 2*halfWidth + 1;
        const f32 boxArea   = static_cast<f32>(boxWidth*boxWidth);
        const f32 outMean = 128.f; //static_cast<f32>(GetImageTypeMean<u8>());

        for(s32 y=0; y<imageHeight; y++) {
          // Input/Output pixel pointers:
          const u8 * restrict pImageRow     = image.Pointer(y,0);
          u8       * restrict pImageRowNorm = imageNorm.Pointer(y,0);

          // Integral image pointers for top and bottom of summing box
          s32 rowAhead  = y + halfWidth;
          s32 rowBehind = y - halfWidth - 1;
          s32 inBoundsHeight = boxWidth;
          if(rowAhead >= imageHeight) {
            inBoundsHeight = imageHeight - y + halfWidth;
            rowAhead = imageHeight - 1;
          }
          if(rowBehind < 0) {
            inBoundsHeight = y+halfWidth+1;
            rowBehind = 0;
          }

          const f32 * restrict pIntegralImageRowBehind = integralImage.Pointer(rowBehind,0);
          const f32 * restrict pIntegralImageRowAhead  = integralImage.Pointer(rowAhead, 0);

          // Left side
          for(s32 x=0; x<=halfWidth; x++) {
            f32 OutOfBoundsArea = static_cast<f32>(boxArea - (x+halfWidth+1)*inBoundsHeight);

            f32 boxSum = (pIntegralImageRowAhead[x+halfWidth] -
              pIntegralImageRowAhead[0] -
              pIntegralImageRowBehind[x+halfWidth] +
              pIntegralImageRowBehind[0] +
              OutOfBoundsArea*static_cast<f32>(padValue));

            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }

          // Middle
          const f32 OutOfBoundsArea = static_cast<f32>(boxArea - boxWidth*inBoundsHeight);
          const f32 paddingSum = OutOfBoundsArea * static_cast<f32>(padValue);
          for(s32 x=halfWidth+1; x<imageWidth-halfWidth; x++) {
            f32 boxSum = (pIntegralImageRowAhead[x+halfWidth] -
              pIntegralImageRowAhead[x-halfWidth-1] -
              pIntegralImageRowBehind[x+halfWidth] +
              pIntegralImageRowBehind[x-halfWidth-1] +
              paddingSum);

            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }

          // Right side
          for(s32 x=imageWidth-halfWidth; x<imageWidth; x++) {
            f32 OutOfBoundsArea = static_cast<f32>(boxArea - (imageWidth-x+halfWidth)*inBoundsHeight);

            f32 boxSum = (pIntegralImageRowAhead[imageWidth-1] -
              pIntegralImageRowAhead[x-halfWidth-1] -
              pIntegralImageRowBehind[imageWidth-1] +
              pIntegralImageRowBehind[x-halfWidth-1] +
              OutOfBoundsArea*static_cast<f32>(padValue));

            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }
        }

        return RESULT_OK;
      } // BoxFilterNormalize()

      template<> Result BoxFilter<u8,u16,u16>(const Array<u8> &image, const s32 boxHeight, const s32 boxWidth, Array<u16> &filtered, MemoryStack scratch)
      {
        AnkiConditionalErrorAndReturnValue(image.IsValid() && filtered.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "BoxFilter", "Image is invalid");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth  = image.get_size(1);

        const s32 boxHeight2 = boxHeight / 2;
        const s32 boxWidth2 = boxWidth / 2;

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, filtered),
          RESULT_FAIL_INVALID_SIZE, "BoxFilter", "Output normalized image must match input image's size.");

        AnkiConditionalErrorAndReturnValue(imageWidth%8 == 0,
          RESULT_FAIL_INVALID_SIZE, "BoxFilter", "Image width must be divisible by 8");

        AnkiConditionalErrorAndReturnValue(boxHeight > 2 && boxWidth > 2 && IsOdd(boxWidth) && IsOdd(boxHeight),
          RESULT_FAIL_INVALID_SIZE, "BoxFilter", "Box filter must be greater than two and odd");

        AnkiConditionalWarn(boxHeight*boxWidth <= 256,
          "BoxFilter", "Filtering may overflow");

        s32 y;

        // Includes extra padding for simd
        u16 * restrict verticalAccumulator = reinterpret_cast<u16*>( scratch.Allocate(imageWidth*sizeof(u16) + 16) );
        memset(verticalAccumulator, 0, imageWidth*sizeof(u16));

        // Accumulate a whole boxHeight
        for(y=0; y<boxHeight; y++) {
          const u8 * restrict pImage = image.Pointer(y,0);
          for(s32 x=0; x<imageWidth; x+=8) {
            const u32 image3210 = *reinterpret_cast<const u32*>(pImage + x);
            const u32 image7654 = *reinterpret_cast<const u32*>(pImage + x + 4);

            const u32 toAdd10 =  (image3210 & 0xFF)            | ((image3210 & 0xFF00)     << 8);
            const u32 toAdd32 = ((image3210 & 0xFF0000) >> 16) | ((image3210 & 0xFF000000) >> 8);

            const u32 toAdd54 =  (image7654 & 0xFF)            | ((image7654 & 0xFF00)     << 8);
            const u32 toAdd76 = ((image7654 & 0xFF0000) >> 16) | ((image7654 & 0xFF000000) >> 8);

            *reinterpret_cast<u32*>(verticalAccumulator + x)     += toAdd10;
            *reinterpret_cast<u32*>(verticalAccumulator + x + 2) += toAdd32;
            *reinterpret_cast<u32*>(verticalAccumulator + x + 4) += toAdd54;
            *reinterpret_cast<u32*>(verticalAccumulator + x + 6) += toAdd76;
          }
        }

        //
        // Add the first row to the filtered image
        //

        filtered(0,boxHeight2-1,0,-1).Set(0);

        {
          // Grab the pointer to the horizontally negative-offset location in the filtered image
          u16 * restrict pFiltered = filtered.Pointer(boxHeight2,0) - boxWidth2;

          u16 horizontalAccumulator = 0;

          s32 x;
          for(x=0; x<boxWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x];
          }

          filtered(boxHeight2,boxHeight2,0,boxWidth2-1).Set(0);

          pFiltered[x-1] = horizontalAccumulator;

          for(; x<imageWidth-3; x+=4) {
            const u32 toAdd10 = *reinterpret_cast<const u32*>(verticalAccumulator + x);
            const u32 toAdd32 = *reinterpret_cast<const u32*>(verticalAccumulator + x + 2);

            u32 toSub10 = *reinterpret_cast<const u32*>(verticalAccumulator + x - boxWidth);
            u32 toSub32 = *reinterpret_cast<const u32*>(verticalAccumulator + x - boxWidth + 2);

            // h is previous horizontal accumulator
            u32 total10 = toAdd10 + horizontalAccumulator; // [1, 0h]
            total10 += total10 << 16; // [10h, 0h]

            toSub10 += toSub10 << 16; // [10, 0]
            total10 -= toSub10;

            u32 total32 = toAdd32 + (total10 >> 16); // [3, 210h]
            total32 += total32 << 16; // [3210h, 210h]

            toSub32 += toSub32 << 16; // [32, 2]
            total32 -= toSub32;

            horizontalAccumulator = total32 >> 16;

            *reinterpret_cast<u32*>(pFiltered + x) = total10;
            *reinterpret_cast<u32*>(pFiltered + x + 2) = total32;
          }

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
          u16 * restrict pFiltered = filtered.Pointer(y - boxHeight2,0) - boxWidth2;

          const u8 * restrict pImageOld = image.Pointer(y-boxHeight,0);
          const u8 * restrict pImageNew = image.Pointer(y,0);

          for(s32 x=0; x<imageWidth; x+=4) {
            const u32 imageNew3210 = *reinterpret_cast<const u32*>(pImageNew + x);

            const u32 imageOld3210 = *reinterpret_cast<const u32*>(pImageOld + x);

            const u32 toAdd10 =  (imageNew3210 & 0xFF)            | ((imageNew3210 & 0xFF00)     << 8);
            const u32 toAdd32 = ((imageNew3210 & 0xFF0000) >> 16) | ((imageNew3210 & 0xFF000000) >> 8);

            const u32 toSub10 =  (imageOld3210 & 0xFF)            | ((imageOld3210 & 0xFF00)     << 8);
            const u32 toSub32 = ((imageOld3210 & 0xFF0000) >> 16) | ((imageOld3210 & 0xFF000000) >> 8);

            *reinterpret_cast<u32*>(verticalAccumulator + x)     += toAdd10 - toSub10;
            *reinterpret_cast<u32*>(verticalAccumulator + x + 2) += toAdd32 - toSub32;
          }

          u16 horizontalAccumulator = 0;

          s32 x;
          for(x=0; x<boxWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x];
          }

          filtered(y-boxHeight2,y-boxHeight2,0,boxWidth2-1).Set(0);

          pFiltered[x-1] = horizontalAccumulator;

          for(; x<imageWidth-3; x+=4) {
            const u32 toAdd10 = *reinterpret_cast<const u32*>(verticalAccumulator + x);
            const u32 toAdd32 = *reinterpret_cast<const u32*>(verticalAccumulator + x + 2);

            u32 toSub10 = *reinterpret_cast<const u32*>(verticalAccumulator + x - boxWidth);
            u32 toSub32 = *reinterpret_cast<const u32*>(verticalAccumulator + x - boxWidth + 2);

            // h is previous horizontal accumulator
            u32 total10 = toAdd10 + horizontalAccumulator; // [1, 0h]
            total10 += total10 << 16; // [10h, 0h]

            toSub10 += toSub10 << 16; // [10, 0]
            total10 -= toSub10;

            u32 total32 = toAdd32 + (total10 >> 16); // [3, 210h]
            total32 += total32 << 16; // [3210h, 210h]

            toSub32 += toSub32 << 16; // [32, 2]
            total32 -= toSub32;

            horizontalAccumulator = total32 >> 16;

            *reinterpret_cast<u32*>(pFiltered + x) = total10;
            *reinterpret_cast<u32*>(pFiltered + x + 2) = total32;
          }

          for(; x<imageWidth; x++) {
            horizontalAccumulator += verticalAccumulator[x] - verticalAccumulator[x-boxWidth];
            pFiltered[x] = horizontalAccumulator;
          }

          filtered(y-boxHeight2,y-boxHeight2,-boxWidth2,-1).Set(0);
        }

        filtered(-boxHeight2,-1,0,-1).Set(0);

        return RESULT_OK;
      } // Result BoxFilter()

      Result FastGradient(const Array<u8> &in, Array<s8> &dx, Array<s8> &dy, MemoryStack scratch)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid() && dy.IsValid() && dx.IsValid() && scratch.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "FastGradient", "Image is invalid");

        AnkiConditionalErrorAndReturnValue(
          imageHeight == dx.get_size(0) && imageHeight == dy.get_size(0) &&
          imageWidth == dx.get_size(1) && imageWidth == dy.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "FastGradient", "Images must be the same size");

        AnkiConditionalErrorAndReturnValue(imageWidth%8 == 0,
          RESULT_FAIL_INVALID_SIZE, "FastGradient", "Image width must be divisible by 8");

        dx(0,0,0,-1).Set(0);

        for(s32 y=1; y<(imageHeight-1); y++) {
          const u8 * restrict pIn_y0  = in.Pointer(y,0);

          s8 * restrict pDx = dx.Pointer(y,0);

          s32 x;

          //#if !defined(USE_ARM_ACCELERATION)
          for(x=1; x<(imageWidth-1); x++) {
            pDx[x] = static_cast<s8>( (static_cast<s32>(pIn_y0[x+1]) >> 1) - (static_cast<s32>(pIn_y0[x-1]) >> 1) );
          }
          //#else // #if !defined(USE_ARM_ACCELERATION)
          //          // TODO: make work so Keil doesn't merge loads
          //          for(x = 0; x<(imageWidth-7); x+=8) {
          //            const u32 inM3210 = *reinterpret_cast<const u32*>(pIn_y0 + x - 1);
          //            const u32 inM7654 = *reinterpret_cast<const u32*>(pIn_y0 + x + 3);
          //
          //            const u32 inP3210 = *reinterpret_cast<const u32*>(pIn_y0 + x + 1);
          //            const u32 inP7654 = *reinterpret_cast<const u32*>(pIn_y0 + x + 5);
          //
          //            const u32 inM3210Half = (inM3210 >> 1) & 0x7f7f7f7f;
          //            const u32 inM7654Half = (inM7654 >> 1) & 0x7f7f7f7f;
          //
          //            const u32 inP3210Half = (inP3210 >> 1) & 0x7f7f7f7f;
          //            const u32 inP7654Half = (inP7654 >> 1) & 0x7f7f7f7f;
          //
          //            const u32 out3210 = __SSUB8(inP3210Half, inM3210Half);
          //            const u32 out7654 = __SSUB8(inP7654Half, inM7654Half);
          //
          //            *reinterpret_cast<u32*>(pDx + x) = out3210;
          //            *reinterpret_cast<u32*>(pDx + x + 4) = out7654;
          //          }
          //#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

          pDx[0] = 0;
          pDx[imageWidth-1] = 0;

          const u8 * restrict pIn_ym1 = in.Pointer(y-1,0);
          const u8 * restrict pIn_yp1 = in.Pointer(y+1,0);

          s8 * restrict pDy = dy.Pointer(y,0);

#if !defined(USE_ARM_ACCELERATION)
          for(x=1; x<(imageWidth-1); x++) {
            pDy[x] = static_cast<s8>( (static_cast<s32>(pIn_yp1[x]) >> 1) - (static_cast<s32>(pIn_ym1[x]) >> 1) );
          }
#else // #if !defined(USE_ARM_ACCELERATION)
          // pIn_ym1 and pIn_yp1 should always be aligned, so this is okay even with auto-load-merging
          for(x = 0; x<(imageWidth-7); x+=8) {
            const u32 inM3210 = *reinterpret_cast<const u32*>(pIn_ym1 + x);
            const u32 inM7654 = *reinterpret_cast<const u32*>(pIn_ym1 + x + 4);

            const u32 inP3210 = *reinterpret_cast<const u32*>(pIn_yp1 + x);
            const u32 inP7654 = *reinterpret_cast<const u32*>(pIn_yp1 + x + 4);

            const u32 inM3210Half = (inM3210 >> 1) & 0x7f7f7f7f;
            const u32 inM7654Half = (inM7654 >> 1) & 0x7f7f7f7f;

            const u32 inP3210Half = (inP3210 >> 1) & 0x7f7f7f7f;
            const u32 inP7654Half = (inP7654 >> 1) & 0x7f7f7f7f;

            const u32 out3210 = __SSUB8(inP3210Half, inM3210Half);
            const u32 out7654 = __SSUB8(inP7654Half, inM7654Half);

            *reinterpret_cast<u32*>(pDy + x) = out3210;
            *reinterpret_cast<u32*>(pDy + x + 4) = out7654;
          }
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

          pDy[0] = 0;
          pDy[imageWidth-1] = 0;
        } // for(s32 y=1; y<(imageHeight-1); y++)

        dx(-1,-1,0,-1).Set(0);

        return RESULT_OK;
      } // Result FastGradient(const Array<u8> &in, Array<s8> &dx, Array<s8> &dy, MemoryStack scratch)
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki
