/**
File: cameraImagingPipeline.cpp
Author: Peter Barnum
Created: 2014-04-24

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/hostIntrinsics_m4.h"

#include "anki/vision/robot/cameraImagingPipeline.h"
#include "anki/vision/robot/histogram.h"

#define USE_ARM_ACCELERATION

#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif

namespace Anki
{
  namespace Embedded
  {
    Result ComputeBestCameraParameters(
      const Array<u8> &image,
      const Rectangle<s32> &imageRegionOfInterest,
      const s32 integerCountsIncrement,
      const f32 percentileToSaturate,
      const f32 minMilliseconds,
      const f32 maxMilliseconds,
      f32 &exposureMilliseconds,
      MemoryStack scratch)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth  = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(integerCountsIncrement > 0 && exposureMilliseconds > 0.0f && percentileToSaturate >= 0.0f && percentileToSaturate <= 1.0f,
        RESULT_FAIL_INVALID_PARAMETER, "ComputeBestCameraParameters", "Invalid parameters");

      IntegerCounts counts(image, imageRegionOfInterest, integerCountsIncrement, integerCountsIncrement, scratch);

      const f32 oldExposureMilliseconds = exposureMilliseconds;

      const f32 numElements = static_cast<f32>(counts.get_numElements());
      const f32 numCurrentlySaturated = static_cast<f32>(counts.get_counts()[255]);
      const f32 percentCurrentlySaturated = numCurrentlySaturated / numElements;

      if(percentCurrentlySaturated >= (1.0f - percentileToSaturate)) {
        // If the brightness is too high, we have to guess at the correct brightness

        // If we assume a underlying uniform distribution of true irradience,
        // then the ratio of saturated to unsaturated pixels tells us how to change the threshold
        exposureMilliseconds = oldExposureMilliseconds * ((1.0f - percentileToSaturate) / percentCurrentlySaturated);
      } else {
        // If the brightness is too low, compute the new brightness exactly
        const f32 curBrightPixelValue = static_cast<f32>( MAX(1, counts.ComputePercentile(percentileToSaturate)) );

        exposureMilliseconds = oldExposureMilliseconds * (255.0f / curBrightPixelValue);
      }

      exposureMilliseconds = CLIP(exposureMilliseconds, minMilliseconds, maxMilliseconds);

      return RESULT_OK;
    }

    Result CorrectVignetting(
      Array<u8> &image,
      const FixedLengthList<f32> &polynomialParameters)
    {
      // Matlab equivalent: x = repmat(1:2:16,[2,1]); x = x(:)'; y = 1; min(255,floor(128*(1 + .01*x + .03*y + .01*x.^2 - .01*y.^2)))

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(IsOdd(polynomialParameters.get_size()) && polynomialParameters.get_size() == 5,
        RESULT_FAIL_INVALID_PARAMETER, "CorrectVignetting", "only 5 polynomialParameters currently supported (2nd order polynomial)");

      AnkiConditionalErrorAndReturnValue((imageWidth%4 == 0) && imageWidth >= 4 && (imageHeight%4 == 0) && imageHeight >= 4,
        RESULT_FAIL_INVALID_SIZE, "CorrectVignetting", "Image width and height must be divisible by 4");

      const s32 polynomialDegree = (polynomialParameters.get_size() - 1) / 2;

      const s32 imageWidth4 = imageWidth / 4;

      const f32 model0 = polynomialParameters[0];
      const f32 model1 = polynomialParameters[1];
      const f32 model2 = polynomialParameters[2];
      const f32 model3 = polynomialParameters[3];
      const f32 model4 = polynomialParameters[4];

      // Computes one scale for every 2x2 block of pixels

      f32 yF32 = 1.0f;
      for(s32 y=0; y<imageHeight; y+=2) {
        u32 * restrict pImage_y0 = reinterpret_cast<u32*>(image.Pointer(y,0));
        u32 * restrict pImage_y1 = reinterpret_cast<u32*>(image.Pointer(y+1,0));

        const f32 yScaleComponent = model0 + model2 * yF32 + model4 * yF32 * yF32;

        // Unrolled to compute two 2x2 blocks at a time
        f32 xF32 = 1.0f;
        for(s32 x=0; x<imageWidth4; x++) {
          const u32 curPixels_y0 = pImage_y0[x];
          const u32 curPixels_y1 = pImage_y1[x];

          const f32 curPixel_y0_x0 = static_cast<f32>( curPixels_y0 & 0xFF);
          const f32 curPixel_y0_x1 = static_cast<f32>((curPixels_y0 & 0xFF00) >> 8);
          const f32 curPixel_y0_x2 = static_cast<f32>((curPixels_y0 & 0xFF0000) >> 16);
          const f32 curPixel_y0_x3 = static_cast<f32>((curPixels_y0 & 0xFF000000) >> 24);

          const f32 curPixel_y1_x0 = static_cast<f32>( curPixels_y1 & 0xFF);
          const f32 curPixel_y1_x1 = static_cast<f32>((curPixels_y1 & 0xFF00) >> 8);
          const f32 curPixel_y1_x2 = static_cast<f32>((curPixels_y1 & 0xFF0000) >> 16);
          const f32 curPixel_y1_x3 = static_cast<f32>((curPixels_y1 & 0xFF000000) >> 24);

          const f32 xF32_left  = xF32;
          const f32 xF32_right = xF32 + 2.0f;

          const f32 scale_left  = yScaleComponent + model1 * xF32_left + model3 * xF32_left * xF32_left;
          const f32 scale_right = yScaleComponent + model1 * xF32_right + model3 * xF32_right * xF32_right;

          const f32 curScaledPixelF32_y0_x0 = scale_left * curPixel_y0_x0;
          const f32 curScaledPixelF32_y0_x1 = scale_left * curPixel_y0_x1;

          const f32 curScaledPixelF32_y1_x0 = scale_left * curPixel_y1_x0;
          const f32 curScaledPixelF32_y1_x1 = scale_left * curPixel_y1_x1;

          const f32 curScaledPixelF32_y0_x2 = scale_right * curPixel_y0_x2;
          const f32 curScaledPixelF32_y0_x3 = scale_right * curPixel_y0_x3;

          const f32 curScaledPixelF32_y1_x2 = scale_right * curPixel_y1_x2;
          const f32 curScaledPixelF32_y1_x3 = scale_right * curPixel_y1_x3;

          const u32 curScaledPixelU32_y0_x0 = static_cast<u32>(curScaledPixelF32_y0_x0);
          const u32 curScaledPixelU32_y0_x1 = static_cast<u32>(curScaledPixelF32_y0_x1);
          const u32 curScaledPixelU32_y0_x2 = static_cast<u32>(curScaledPixelF32_y0_x2);
          const u32 curScaledPixelU32_y0_x3 = static_cast<u32>(curScaledPixelF32_y0_x3);

          const u32 curScaledPixelU32_y1_x0 = static_cast<u32>(curScaledPixelF32_y1_x0);
          const u32 curScaledPixelU32_y1_x1 = static_cast<u32>(curScaledPixelF32_y1_x1);
          const u32 curScaledPixelU32_y1_x2 = static_cast<u32>(curScaledPixelF32_y1_x2);
          const u32 curScaledPixelU32_y1_x3 = static_cast<u32>(curScaledPixelF32_y1_x3);

#if !defined(USE_ARM_ACCELERATION) // natural C
          const u8 outPixel_y0_x0 = static_cast<u8>(MIN(255, curScaledPixelU32_y0_x0));
          const u8 outPixel_y0_x1 = static_cast<u8>(MIN(255, curScaledPixelU32_y0_x1));
          const u8 outPixel_y0_x2 = static_cast<u8>(MIN(255, curScaledPixelU32_y0_x2));
          const u8 outPixel_y0_x3 = static_cast<u8>(MIN(255, curScaledPixelU32_y0_x3));

          const u8 outPixel_y1_x0 = static_cast<u8>(MIN(255, curScaledPixelU32_y1_x0));
          const u8 outPixel_y1_x1 = static_cast<u8>(MIN(255, curScaledPixelU32_y1_x1));
          const u8 outPixel_y1_x2 = static_cast<u8>(MIN(255, curScaledPixelU32_y1_x2));
          const u8 outPixel_y1_x3 = static_cast<u8>(MIN(255, curScaledPixelU32_y1_x3));
#else // ARM optimized
          const u8 outPixel_y0_x0 = __USAT(curScaledPixelU32_y0_x0, 8);
          const u8 outPixel_y0_x1 = __USAT(curScaledPixelU32_y0_x1, 8);
          const u8 outPixel_y0_x2 = __USAT(curScaledPixelU32_y0_x2, 8);
          const u8 outPixel_y0_x3 = __USAT(curScaledPixelU32_y0_x3, 8);

          const u8 outPixel_y1_x0 = __USAT(curScaledPixelU32_y1_x0, 8);
          const u8 outPixel_y1_x1 = __USAT(curScaledPixelU32_y1_x1, 8);
          const u8 outPixel_y1_x2 = __USAT(curScaledPixelU32_y1_x2, 8);
          const u8 outPixel_y1_x3 = __USAT(curScaledPixelU32_y1_x3, 8);
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

          const u32 outPixel_y0 = outPixel_y0_x0 | (outPixel_y0_x1<<8) | (outPixel_y0_x2<<16) | (outPixel_y0_x3<<24);
          const u32 outPixel_y1 = outPixel_y1_x0 | (outPixel_y1_x1<<8) | (outPixel_y1_x2<<16) | (outPixel_y1_x3<<24);

          pImage_y0[x] = outPixel_y0;
          pImage_y1[x] = outPixel_y1;

          xF32 += 4.0f;
        }

        yF32 += 2.0f;
      }

      return RESULT_OK;
    } // CorrectVignetting()
  } // namespace Embedded
} // namespace Anki
