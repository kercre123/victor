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
      const s32 integerCountsIncrement, // Can be any value >= 1. Higher values skip more pixels, so are faster and less accurate.
      const f32 percentileToSaturate, // Can be in the range [0.0, 1.0]. 0.95 is a good value
      f32 &exposureMilliseconds, //< Input the value used to capture "Array<u8> &image", and outputs the value to set next
      MemoryStack scratch);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_CAMERA_IMAGING_PIPELINE_H_
