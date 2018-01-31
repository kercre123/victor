/**
File: features.h
Author: Peter Barnum
Created: 2014-10-28

Recognize faces from an image

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

// Deprecate?
#if 0

#ifndef _ANKICORETECHEMBEDDED_VISION_RECOGNIZE_H_
#define _ANKICORETECHEMBEDDED_VISION_RECOGNIZE_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "coretech/vision/robot/transformations.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/contrib/contrib.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace Recognize
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      //Result RecognizeFace(
      //  const Array<u8> &image,
      //  const Rectangle<s32> faceLocation,
      //  cv::CascadeClassifier &eyeClassifier,
      //  const f32 eyeQualityThreshold, //< A value between about 1/2000 to 1/5000 is good, where 0.0 is terrible and 1.0 is wonderful
      //  std::vector<cv::Rect> &detectedEyes,
      //  s32 &leftEyeIndex,
      //  s32 &rightEyeIndex,
      //  f32 &eyeQuality,
      //  s32 &faceId,
      //  f64 &confidence);

      Result RecognizeFace(
        const Array<u8> &image,
        const Rectangle<s32> faceLocation,
        const FixedLengthList<Array<u8> > &trainingImages,
        const FixedLengthList<Rectangle<s32> > &trainingFaceLocations,
        s32 &faceId,
        f64 &confidence,
        MemoryStack scratch);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
    } // namespace Recognize
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_RECOGNIZE_H_

#endif // #if 0
