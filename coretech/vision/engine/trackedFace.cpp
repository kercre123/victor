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

#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/common/engine/math/point_impl.h"

namespace Anki {
namespace Vision {
  
  static const f32 DistanceBetweenEyes_mm = 62.f;
  
  // Assuming a max face detection of 3m, focal length of 300 and distanceBetweenEyes_mm of 62
  // then the smallest distance between eyes in pixels will be ~6
  static const f32 MinDistBetweenEyes_pixels = 6;
  
  TrackedFace::TrackedFace()
  : _smileAmount(false, 0.f, 0.f)
  , _gaze(false, 0.f, 0.f)
  , _blinkAmount(false, 0.f, 0.f)
  , _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  }
  
  f32 TrackedFace::GetIntraEyeDistance() const
  {
    f32 imageDist = (_leftEyeCen - _rightEyeCen).Length();

    f32 yaw  = std::cos(GetHeadYaw().ToFloat());
    
    if(NEAR_ZERO(yaw))
    {
      yaw = 1;
    }
    
    if(NEAR_ZERO(imageDist))
    {
      PRINT_NAMED_WARNING("TrackedFace.GetIntraEyeDistance.ZeroEyeDist",
                          "LeftEyeCen (%f %f) RightEyeCen (%f %f) Dist %f",
                          _leftEyeCen.x(),
                          _leftEyeCen.y(),
                          _rightEyeCen.x(),
                          _rightEyeCen.y(),
                          imageDist);
      imageDist = MinDistBetweenEyes_pixels;
    }
    
    return imageDist / yaw;
  }
  
  bool TrackedFace::UpdateTranslation(const Vision::Camera& camera)
  {
    bool usedRealCenters = true;
    Point2f leftEye, rightEye;
    f32 intraEyeDistance = 0;
    if(!GetEyeCenters(leftEye, rightEye))
    {
      // No eyes set: Use fake eye centers
      DEV_ASSERT(_rect.Area() > 0, "Invalid face rectangle");
      Point2f leftEye(GetRect().GetXmid() - .25f*GetRect().GetWidth(),
                      GetRect().GetYmid() - .125f*GetRect().GetHeight());
      Point2f rightEye(GetRect().GetXmid() + .25f*GetRect().GetWidth(),
                       GetRect().GetYmid() - .125f*GetRect().GetHeight());

      usedRealCenters = false;
      intraEyeDistance = std::max((rightEye - leftEye).Length(), MinDistBetweenEyes_pixels);
    }
    else
    {
      intraEyeDistance = GetIntraEyeDistance();
    }
    
    DEV_ASSERT(!NEAR_ZERO(intraEyeDistance), "IntraEyeDistance is near zero");
      
    // Get unit vector along camera ray from the point between the eyes in the image
    Point2f eyeMidPoint(leftEye);
    eyeMidPoint += rightEye;
    eyeMidPoint *= 0.5f;
    
    Point3f ray(eyeMidPoint.x(), eyeMidPoint.y(), 1.f);
    DEV_ASSERT(camera.IsCalibrated(), "Camera should be calibrated");
    ray = camera.GetCalibration()->GetInvCalibrationMatrix() * ray;
    ray.MakeUnitLength();
    ray *= camera.GetCalibration()->GetFocalLength_x() * DistanceBetweenEyes_mm / intraEyeDistance;
    _headPose.SetTranslation(ray);

    RotationMatrix3d rotation(-GetHeadPitch(), -GetHeadRoll(), GetHeadYaw());
    _headPose.SetRotation(_headPose.GetRotation() * rotation);

    _isTranslationSet = true;
    _headPose.SetParent(camera.GetPose());
    
    return usedRealCenters;
  }
  
  const TrackedFace::FacialExpressionValues& TrackedFace::GetExpressionValues() const
  {
    return _expression;
  }
  
  // Return the expression with highest value
  FacialExpression TrackedFace::GetMaxExpression(s32* valuePtr) const
  {
    static_assert((s32)FacialExpression::Unknown == -1, "Expecting Unknown expression to be value -1");
    
    FacialExpression maxExpression = FacialExpression::Unknown;
    s32 maxValue = -1;
    for(s32 crntExpression = 0; crntExpression < (s32)FacialExpression::Count; ++crntExpression)
    {
      if(_expression[crntExpression] > maxValue) {
        maxValue = _expression[crntExpression];
        maxExpression = (FacialExpression)crntExpression;
      }
    }
    
    if(nullptr != valuePtr)
    {
      *valuePtr = maxValue;
    }
    return maxExpression;
  }
  
  void TrackedFace::SetExpressionValue(FacialExpression whichExpression, f32 newValue)
  {
    const u32 expressionIndex = (u32)whichExpression;
    DEV_ASSERT(expressionIndex < (u32)FacialExpression::Count, "TrackedFace.SetExpressionValue.BadExpression");
    _expression[expressionIndex] = newValue;
  }
  
} // namespace Vision
} // namespace Anki


