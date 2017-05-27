/**
 * File: visionPoseData.cpp
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Container for passing around pose/state information from a certain
 *              timestamp, useful for vision processing.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "visionPoseData.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

bool VisionPoseData::IsBodyPoseSame(const VisionPoseData& other, const Radians& bodyAngleThresh,
                                    const f32 bodyPoseThresh_mm) const
{
  const bool isXPositionSame = NEAR(histState.GetPose().GetTranslation().x(),
                                    other.histState.GetPose().GetTranslation().x(),
                                    bodyPoseThresh_mm);
  
  const bool isYPositionSame = NEAR(histState.GetPose().GetTranslation().y(),
                                    other.histState.GetPose().GetTranslation().y(),
                                    bodyPoseThresh_mm);
  
  const bool isAngleSame     = NEAR(histState.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat(),
                                    other.histState.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat(),
                                    bodyAngleThresh.ToFloat());
  
  const bool isPoseSame = isXPositionSame && isYPositionSame && isAngleSame;
  
  return isPoseSame;
}

bool VisionPoseData::IsHeadAngleSame(const VisionPoseData& other, const Radians& headAngleThresh) const
{
  const bool headSame =  NEAR(histState.GetHeadAngle_rad(),
                              other.histState.GetHeadAngle_rad(),
                              headAngleThresh.ToFloat());
  
  return headSame;
}

} // namespace Cozmo
} // namespace Anki
