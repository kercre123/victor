/**
 * File: nightVisionAccumulator.h
 *
 * Author: Humphrey Hu
 * Date:   2018-06-08
 *
 * Description: Helper class for averaging images together and contrast adjusting them.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Victor_NightVisionAccumulator_H__
#define __Anki_Victor_NightVisionAccumulator_H__

#include "coretech/common/engine/array2d.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {
namespace Cozmo {

class NightVisionAccumulator
{
public:

  NightVisionAccumulator(const Json::Value& config);

  void AddImage( const Vision::Image& img, const VisionPoseData& poseData );
  void Reset();

  // Generates a contrast-adjusted composite image from the accumulated images
  bool GetOutput( Vision::Image& out ) const;

private:

  using ImageAcc = Array2d<u32>;
  ImageAcc _accumulator;
  u32 _numAccImages;

  u32 _minNumImages;
  s32 _histSubsample;
  f32 _contrastTargetPercentile;
  u8 _contrastTargetValue;
};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Anki_Victor_NightVisionAccumulator_H__