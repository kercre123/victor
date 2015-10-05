/**
 * File: trackedFace.cpp
 *
 * Author: Andrew Stein
 * Date:   8/20/2015
 *
 * Description: A container for a tracked face and any features (e.g. eyes, mouth, ...)
 *              related to it.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/basestation/camera.h"
#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Vision {
  
  static const f32 DistanceBetweenEyes_mm = 62.f;
  
  TrackedFace::TrackedFace()
  : _id(-1)
  , _name("")
  , _isBeingTracked(false)
  , _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  }
  
  f32 TrackedFace::GetIntraEyeDistance() const
  {
    const f32 imageDist = (GetLeftEyeCenter() - GetRightEyeCenter()).Length();
    
    const f32 roll = GetHeadRoll().ToFloat();
    const f32 yaw  = GetHeadYaw().ToFloat();
    
    return imageDist / std::cos(roll) / std::cos(yaw);
  }
  
  void TrackedFace::UpdateTranslation(const Vision::Camera& camera)
  {
    // Get unit vector along camera ray from the point between the eyes in the image
    Point2f eyeMidPoint(GetLeftEyeCenter());
    eyeMidPoint += GetRightEyeCenter();
    eyeMidPoint *= 0.5f;
    
    Point3f ray(eyeMidPoint.x(), eyeMidPoint.y(), 1.f);
    ray = camera.GetCalibration().GetInvCalibrationMatrix() * ray;
    ray.MakeUnitLength();
    
    ray *= camera.GetCalibration().GetFocalLength_x() * DistanceBetweenEyes_mm / GetIntraEyeDistance();
    
    _headPose.SetTranslation(ray);
    _headPose.SetParent(&camera.GetPose());
  }
  
} // namespace Vision
} // namespace Anki


