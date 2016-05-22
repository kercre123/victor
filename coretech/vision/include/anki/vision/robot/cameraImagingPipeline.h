/**
File: cameraImagingPipeline.h
Author: Peter Barnum
Created: 2014-04-24

Compute camera settings, and eventually, perhaps handle things like vignetting correction and noise reduction

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_CAMERA_IMAGING_PIPELINE_H_
#define _ANKICORETECHEMBEDDED_VISION_CAMERA_IMAGING_PIPELINE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    // Based on an input image, compute statistics on the image, and compute how to change the
    // camera parameters next frame
    Result ComputeBestCameraParameters(
      const Array<u8> &image,
      const Rectangle<s32> &imageRegionOfInterest,
      const s32 integerCountsIncrement, // Can be any value >= 1. Higher values skip more pixels, so are faster and less accurate. 3 is a good value.
      const u8 highValue, // Values equal-to or greater than highValue are considered high. 250 is a good value.
      const f32 percentileToMakeHigh, // What percentile of brightness should be set to highValue? Can be in the range [0.0, 1.0]. 0.95 is a good value
      const f32 minExposure, // What is the minimum exposure time to clip the output to? 0.03 is a good value.
      const f32 maxExposure, // What is the maximum exposure time to clip the output to? 0.97 is a good value.
      const f32 tooHighPercentMultiplier, // If the exposure is too high, what is the maximum percent reduction in exposure time? Higher values mean the exposure will adapt more slowly. Can be in the range [0.0, 1.0]. 0.8 is a good value
      f32 &exposureTime, //< Input the value used to capture "Array<u8> &image", and outputs the value to set next
      MemoryStack scratch);

    // Scale the grayvalue of each pixel, to correct for vignetting
    // Use the matlab tool fit2dCurve to get a model of the form [constant, x, y, x.^2, y.^2, ...]
    // The model must be such that the scale at every pixel is in the range [0.0, 2^8];
    // NOTE: The rounding is approximate, so the answer may be 1 different than Matlab
    Result CorrectVignetting(
      Array<u8> &image,
      const FixedLengthList<f32> &polynomialParameters);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_CAMERA_IMAGING_PIPELINE_H_
