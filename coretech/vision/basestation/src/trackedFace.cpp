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
  , _isBeingTracked(false)
  , _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  }
  
  f32 TrackedFace::GetIntraEyeDistance() const
  {
    const f32 imageDist = (_leftEyeCen - _rightEyeCen).Length();
    
    const f32 roll = GetHeadRoll().ToFloat();
    const f32 yaw  = GetHeadYaw().ToFloat();
    
    return imageDist / std::cos(roll) / std::cos(yaw);
  }
  
  bool TrackedFace::UpdateTranslation(const Vision::Camera& camera)
  {
    bool usedRealCenters = true;
    Point2f leftEye, rightEye;
    if(!GetEyeCenters(leftEye, rightEye))
    {
      // No eyes set: Use fake eye centers
      ASSERT_NAMED(_rect.Area() > 0, "Invalid face rectangle");
      Point2f leftEye(GetRect().GetXmid() - .25f*GetRect().GetWidth(),
                      GetRect().GetYmid() - .125f*GetRect().GetHeight());
      Point2f rightEye(GetRect().GetXmid() + .25f*GetRect().GetWidth(),
                       GetRect().GetYmid() - .125f*GetRect().GetHeight());

      usedRealCenters = false;
    }
      
    // Get unit vector along camera ray from the point between the eyes in the image
    Point2f eyeMidPoint(leftEye);
    eyeMidPoint += rightEye;
    eyeMidPoint *= 0.5f;
    
    Point3f ray(eyeMidPoint.x(), eyeMidPoint.y(), 1.f);
    ASSERT_NAMED(camera.IsCalibrated(), "Camera should be calibrated");
    ray = camera.GetCalibration()->GetInvCalibrationMatrix() * ray;
    ray.MakeUnitLength();
    
    ray *= camera.GetCalibration()->GetFocalLength_x() * DistanceBetweenEyes_mm / GetIntraEyeDistance();
    
    _headPose.SetTranslation(ray);
    _headPose.SetParent(&camera.GetPose());
    
    return usedRealCenters;
  }
  
  std::array<f32, TrackedFace::NumExpressions> TrackedFace::GetExpressionValues() const
  {
    return _expression;
  }
  
  // Return the expression with highest value
  TrackedFace::Expression TrackedFace::GetMaxExpression() const
  {
    Expression maxExpression = (Expression)0;
    f32 maxValue = _expression[0];
    for(s32 crntExpression = 1; crntExpression < NumExpressions; ++crntExpression)
    {
      if(_expression[crntExpression] > maxValue) {
        maxValue = _expression[crntExpression];
        maxExpression = (Expression)crntExpression;
      }
    }
    return maxExpression;
  }
  
  void TrackedFace::SetExpressionValue(Expression whichExpression, f32 newValue)
  {
    _expression[whichExpression] = newValue;
  }
  
  const char* TrackedFace::GetExpressionName(Expression whichExpression)
  {
    static const std::map<Expression, const char*> NameLUT{
      {Neutral,    "Neutral"},
      {Happiness,  "Happineess"},
      {Surprise,   "Surprise"},
      {Anger,      "Anger"},
      {Sadness,    "Sadness"},
    };
    
    auto iter = NameLUT.find(whichExpression);
    if(iter != NameLUT.end()) {
      return iter->second;
    } else {
      return "Unknown";
    }
  }
  
} // namespace Vision
} // namespace Anki


