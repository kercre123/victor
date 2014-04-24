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

      if(percentCurrentlySaturated >= percentileToSaturate) {
        // If the brightness is too high, we have to guess at the correct brightness

        // If we assume a underlying uniform distribution of true irradience,
        // then the ratio of saturated to unsaturated pixels tells us how to change the threshold
        exposureMilliseconds = oldExposureMilliseconds * (percentileToSaturate / percentCurrentlySaturated);
      } else {
        // If the brightness is too low, compute the new brightness exactly
        const f32 curBrightPixelValue = static_cast<f32>( counts.ComputePercentile(percentileToSaturate) );

        exposureMilliseconds = oldExposureMilliseconds * (255.0f / curBrightPixelValue);
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki
