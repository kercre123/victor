/**
File: cameraImagingPipeline.cpp
Author: Peter Barnum
Created: 2014-04-24

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/vision/robot/cameraImagingPipeline.h"
#include "anki/vision/robot/histogram.h"

#define USE_ARM_ACCELERATION

#if defined(__EDG__)
#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif
#else
#undef USE_ARM_ACCELERATION
#endif

#if defined(USE_ARM_ACCELERATION)
#ifdef USING_CHIP_SIMULATOR
#include <ARMCM4.h>
#else
#include <stm32f4xx.h>
#endif
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
      AnkiConditionalErrorAndReturnValue(IsOdd(polynomialParameters.get_size()) && polynomialParameters.get_size() == 5,
        RESULT_FAIL_INVALID_PARAMETER, "CorrectVignetting", "only 5 polynomialParameters currently supported (2nd order polynomial)");

      AnkiConditionalErrorAndReturnValue(image.get_size(1)%4 == 0,
        RESULT_FAIL_INVALID_SIZE, "CorrectVignetting", "Image width must be divisible by 4");

      const s32 polynomialDegree = (polynomialParameters.get_size() - 1) / 2;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      const s32 imageWidth4 = imageWidth / 4;

      const f32 model0 = polynomialParameters[0];
      const f32 model1 = polynomialParameters[1];
      const f32 model2 = polynomialParameters[2];
      const f32 model3 = polynomialParameters[3];
      const f32 model4 = polynomialParameters[4];

      f32 yF32 = 0.5f;
      for(s32 y=0; y<imageHeight; y++) {
        u32 * restrict pImage = reinterpret_cast<u32*>(image.Pointer(y,0));

        const f32 yScaleComponent = model0 + model2 * yF32 + model4 * yF32 * yF32;

        f32 xF32 = 0.5f;
        for(s32 x=0; x<imageWidth4; x++) {
          const u32 curPixels = pImage[x];

          const f32 curPixel_0 = static_cast<f32>(curPixels & 0xFF);
          const f32 curPixel_1 = static_cast<f32>((curPixels & 0xFF00) >> 8);
          const f32 curPixel_2 = static_cast<f32>((curPixels & 0xFF0000) >> 16);
          const f32 curPixel_3 = static_cast<f32>((curPixels & 0xFF000000) >> 24);

          const f32 xF32_0 = xF32;
          const f32 xF32_1 = xF32 + 1.0f;
          const f32 xF32_2 = xF32 + 2.0f;
          const f32 xF32_3 = xF32 + 3.0f;

          const f32 scale_0 = yScaleComponent + model1 * xF32_0 + model3 * xF32_0 * xF32_0;
          const f32 scale_1 = yScaleComponent + model1 * xF32_1 + model3 * xF32_1 * xF32_1;
          const f32 scale_2 = yScaleComponent + model1 * xF32_2 + model3 * xF32_2 * xF32_2;
          const f32 scale_3 = yScaleComponent + model1 * xF32_3 + model3 * xF32_3 * xF32_3;

          const f32 curScaledPixelF32_0 = scale_0 * curPixel_0;
          const f32 curScaledPixelF32_1 = scale_1 * curPixel_1;
          const f32 curScaledPixelF32_2 = scale_2 * curPixel_2;
          const f32 curScaledPixelF32_3 = scale_3 * curPixel_3;

          const u32 curScaledPixelU32_0 = static_cast<u32>(curScaledPixelF32_0);
          const u32 curScaledPixelU32_1 = static_cast<u32>(curScaledPixelF32_1);
          const u32 curScaledPixelU32_2 = static_cast<u32>(curScaledPixelF32_2);
          const u32 curScaledPixelU32_3 = static_cast<u32>(curScaledPixelF32_3);

#if !defined(USE_ARM_ACCELERATION) // natural C
          const u8 outPixel_0 = static_cast<u8>(MIN(255, curScaledPixelU32_0));
          const u8 outPixel_1 = static_cast<u8>(MIN(255, curScaledPixelU32_1));
          const u8 outPixel_2 = static_cast<u8>(MIN(255, curScaledPixelU32_2));
          const u8 outPixel_3 = static_cast<u8>(MIN(255, curScaledPixelU32_3));
#else // ARM optimized
          const u8 outPixel_0 = __USAT(curScaledPixelU32_0, 8);
          const u8 outPixel_1 = __USAT(curScaledPixelU32_1, 8);
          const u8 outPixel_2 = __USAT(curScaledPixelU32_2, 8);
          const u8 outPixel_3 = __USAT(curScaledPixelU32_3, 8);
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

          const u32 outPixel = outPixel_0 | (outPixel_1<<8) | (outPixel_2<<16) | (outPixel_3<<24);

          pImage[x] = outPixel;

          xF32 += 4.0f;
        }

        yF32 += 1.0f;
      }

      return RESULT_OK;
    } // CorrectVignetting()
  } // namespace Embedded
} // namespace Anki
