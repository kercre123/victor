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
  
  // Assuming a max face detection of 3m, focal length of 300 and distanceBetweenEyes_mm of 62
  // then the smallest distance between eyes in pixels will be ~6
  static const f32 MinDistBetweenEyes_pixels = 6;
  
  TrackedFace::TrackedFace()
  : _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
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
      ASSERT_NAMED(_rect.Area() > 0, "Invalid face rectangle");
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
    
    ASSERT_NAMED(!NEAR_ZERO(intraEyeDistance), "IntraEyeDistance is near zero");
      
    // Get unit vector along camera ray from the point between the eyes in the image
    Point2f eyeMidPoint(leftEye);
    eyeMidPoint += rightEye;
    eyeMidPoint *= 0.5f;
    
    Point3f ray(eyeMidPoint.x(), eyeMidPoint.y(), 1.f);
    ASSERT_NAMED(camera.IsCalibrated(), "Camera should be calibrated");
    ray = camera.GetCalibration()->GetInvCalibrationMatrix() * ray;
    ray.MakeUnitLength();
    
    ray *= camera.GetCalibration()->GetFocalLength_x() * DistanceBetweenEyes_mm / intraEyeDistance;
    
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


