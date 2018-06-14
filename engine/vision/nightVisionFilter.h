/**
 * File: nightVisionFilter.h
 *
 * Author: Humphrey Hu
 * Date:   2018-06-08
 *
 * Description: Helper class for averaging images together to minimize noise.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Victor_NightVisionFilter_H__
#define __Anki_Victor_NightVisionFilter_H__

#include "coretech/common/engine/array2d.h"
#include "coretech/common/shared/radians.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {

namespace Cozmo {

class NightVisionFilter
{
public:

  NightVisionFilter();
  
  // Initialize the filter, returns success
  Result Init(const Json::Value& config);

  // Add an image into the filter accumulator
  void AddImage( const Vision::Image& img, const VisionPoseData& poseData );

  // Reset the filter state
  void Reset();

  // Generates a noise-reduced composite image from the accumulated images
  // Returns false if not enough images accumulated
  bool GetOutput( Vision::Image& out );

private:

  // NOTE We get array allocation failures using u32
  using ImageAcc = Array2d<u16>;
  ImageAcc _accumulator;
  ImageAcc _castImage;
  
  TimeStamp_t _lastTimestamp;
  u32 _numAccImages;
  VisionPoseData _lastPoseData;

  u32 _minNumImages;
  s32 _histSubsample;
  Radians _bodyAngleThresh;
  f32 _bodyPoseThresh;
  Radians _headAngleThresh;

  // Check to see if the robot has moved since the given pose data
  bool HasMoved( const VisionPoseData& poseData );
};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Anki_Victor_NightVisionFilter_H__