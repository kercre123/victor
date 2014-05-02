/**
File: autoExposure.cpp
Author: Peter Barnum
Created: 2014-04-24

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/vision/robot/cameraImagingPipeline.h"
#include "anki/vision/robot/histogram.h"

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
      const FixedLengthList<f32> &polynomialParameters,
      MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(IsOdd(polynomialParameters.get_size()) && polynomialParameters.get_size() == 5,
        RESULT_FAIL_INVALID_PARAMETER, "CorrectVignetting", "only 5 polynomialParameters currently supported (2nd order polynomial)");

      const s32 polynomialDegree = (polynomialParameters.get_size() - 1) / 2;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      const f32 model0 = polynomialParameters[0];
      const f32 model1 = polynomialParameters[1];
      const f32 model2 = polynomialParameters[2];
      const f32 model3 = polynomialParameters[3];
      const f32 model4 = polynomialParameters[4];

      for(s32 y=0; y<imageHeight; y++) {
        u8 * restrict pImage = image.Pointer(y,0);

        const f32 yF32 = static_cast<f32>(y);
        const f32 yScaleComponent = model0 + model2 * yF32 + model4 * yF32 * yF32;

        for(s32 x=0; x<imageWidth; x++) {
          const f32 xF32 = static_cast<f32>(x);

          const f32 curPixel = static_cast<f32>(pImage[x]);

          const f32 scale = yScaleComponent + model1 * xF32 + model3 * xF32 * xF32;

          const u8 outPixel = saturate_cast<u8>(scale * curPixel);

          pImage[x] = outPixel;
        }
      }

      return RESULT_OK;
    } // CorrectVignetting()
  } // namespace Embedded
} // namespace Anki
