/**
 * File: faceNormalDirectedAtRobot3d.cpp
 *
 * Author: Robert Cosgriff
 * Date:   9/5/2018
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 **/
#include "faceNormalDirectedAtRobot3d.h"

#include "coretech/common/shared/types.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR(u32, kHistorySize,                      "Vision.FaceNormalDirectedAtRobot3d",  6);
  CONSOLE_VAR(f32, kInlierDistanceSq,                 "Vision.FaceNormalDirectedAtRobot3d",  100.f);
  CONSOLE_VAR(u32, kMinNumberOfInliers,               "Vision.FaceNormalDirectedAtRobot3d",  2);
  CONSOLE_VAR(u32, kExpireThreshold,                  "Vision.FaceNormalDirectedAtRobot3d",  50);
  CONSOLE_VAR(f32, kInlierXThreshold,                 "Vision.FaceNormalDirectedAtRobot3d",  300.f);
  CONSOLE_VAR(f32, kInlierYThreshold,                 "Vision.FaceNormalDirectedAtRobot3d",  100.f);
  CONSOLE_VAR(f32, kInlierZThreshold,                 "Vision.FaceNormalDirectedAtRobot3d",  20.f);
  CONSOLE_VAR(f32, kShiftOutputPointX_mm,             "Vision.FaceNormalDirectedAtRobot3d",  100.f);
}

FaceNormalDirectedAtRobot3d::FaceNormalDirectedAtRobot3d()
  : _faceDirectionSurfaceAverage(Point3f(FaceDirectionData3d::kDefaultDistance_cm,
                                 FaceDirectionData3d::kDefaultDistance_cm,
                                 FaceDirectionData3d::kDefaultDistance_cm)),
  _eyeDirectionAverage(Point3f(FaceDirectionData3d::kDefaultDistance_cm,
                               FaceDirectionData3d::kDefaultDistance_cm,
                               FaceDirectionData3d::kDefaultDistance_cm))
{
  _faceDirectionSurfaceHistory.resize(kHistorySize);
  _eyeDirectionHistory.resize(kHistorySize);
}

void FaceNormalDirectedAtRobot3d::Update(const TrackedFace& face,
                                         const TimeStamp_t timeStamp)
{
  _headPose = face.GetHeadPose();
  _eyePose = face.GetEyePose();
  _eyePose.Print("FaceNormalDirectedAtRobot3d.Update.EyePose");
  _lastUpdated = timeStamp;

  if (_currentIndex > (kHistorySize - 1))
  {
    // Make sure we place a "real" value
    // in every element of the history
    // before returning a result
    _initialized = true;
    _currentIndex = 0;
  }

  // First add our current point to the history, since these
  // angles are range limited without a wrap around we will
  // be able to treat them as Cartesian coordinates

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.FaceAngles",
                   "yaw=%.3f, pitch=%.3f, roll=%.3f", RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                   RAD_TO_DEG(face.GetHeadPitch().ToFloat()), RAD_TO_DEG(face.GetHeadRoll().ToFloat()));

  Point3f faceDirectionPoint;
  bool include = GetPointFromHeadPose(_headPose, faceDirectionPoint);

  /*
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.YawPitch",
                   "Yaw=%.3f, Pitch=%.3f", RAD_TO_DEG(_face.GetHeadYaw().ToFloat()),
                   RAD_TO_DEG(_face.GetHeadPitch().ToFloat()));
  */
  _faceDirectionSurfaceHistory[_currentIndex].Update(faceDirectionPoint,
                                              RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                                              RAD_TO_DEG(face.GetHeadPitch().ToFloat()),
                                              include);

  _faceDirectionSurfaceAverage = ComputeEntireFaceDirectionAverage();
  _numberOfInliers = FindInliers(_faceDirectionSurfaceAverage);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _faceDirectionSurfaceAverage = RecomputeFaceDirectionAverage();

  Point3f eyeDirectionPoint;
  include = GetPointFromEyePose(_eyePose, eyeDirectionPoint);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.eyeDirectionPoint",
                   "x: %.3f, y:%.3f, z:%.3f", eyeDirectionPoint.x(), eyeDirectionPoint.y(),
                   eyeDirectionPoint.z());
  // TODO i needed to reverse the sign of the yaw direction (left right) direction
  // to agree with the yaw above when we go to calculate the eye gaze point
  _eyeDirectionHistory[_currentIndex].Update(eyeDirectionPoint,
                                             RAD_TO_DEG(-face.GetGaze().leftRight_deg),
                                             RAD_TO_DEG(face.GetGaze().upDown_deg),
                                             include);
  _eyeDirectionAverage = ComputeEntireEyeDirectionAverage();
  _numberOfEyeDirectionsInliers = FindEyeDirectionInliers(_eyeDirectionAverage);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _eyeDirectionAverage = RecomputeEyeDirectionAverage();

  _currentIndex += 1;
}

Point3f FaceNormalDirectedAtRobot3d::ComputeEntireFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(false);
}

Point3f FaceNormalDirectedAtRobot3d::ComputeFaceDirectionAverage(const bool filterOutliers)
{
  Point3f averageFaceDirection = Point3f(0.f, 0.f, 0.f);

  // Find the average, we can treat these as Cartesian
  // coordinates instead traditional angles because the
  // range of value does not include a wrap around
  u32 pointsInAverage = 0;
  for (const auto& faceDirection: _faceDirectionSurfaceHistory) {
    if (faceDirection.include) {
      if (filterOutliers) {
        if (faceDirection.inlier) {
          pointsInAverage += 1;
          averageFaceDirection += faceDirection.point;
        }
      } else {
        pointsInAverage += 1;
        averageFaceDirection += faceDirection.point;
      }
    }
  }

  if (pointsInAverage != 0) {
    averageFaceDirection *= 1.f/pointsInAverage;
  } else {
    averageFaceDirection = Point3f(FaceDirectionData3d::kDefaultDistance_cm,
                                   FaceDirectionData3d::kDefaultDistance_cm,
                                   FaceDirectionData3d::kDefaultDistance_cm);
  }
  return averageFaceDirection;
}

int FaceNormalDirectedAtRobot3d::FindInliers(const Point3f& faceDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& faceDirection: _faceDirectionSurfaceHistory) {
    /*
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.FindInliers.PointHistory",
                     "direction x=%.3f, y=%.3f, z=%.3f", faceDirection.point.x(),
                     faceDirection.point.y(), faceDirection.point.z());
    */

    if (!faceDirection.include) {
      continue;
    }
                      
    auto difference = faceDirection.point - faceDirectionAverage;
    /*
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.FindInliers.Difference",
                     "direction x=%.3f, y=%.3f, z=%.3f", difference.x(),
                     difference.y(), difference.z());
    */
    if (difference.x() < kInlierXThreshold && difference.y() < kInlierYThreshold && difference.z() < kInlierZThreshold) {
      faceDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      faceDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

Point3f FaceNormalDirectedAtRobot3d::RecomputeFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(true);
}

Point3f FaceNormalDirectedAtRobot3d::ComputeEntireEyeDirectionAverage()
{
  return ComputeEyeDirectionAverage(false);
}

Point3f FaceNormalDirectedAtRobot3d::ComputeEyeDirectionAverage(const bool filterOutliers)
{
  Point3f averageEyeDirection = Point3f(0.f, 0.f, 0.f);

  // Find the average, we can treat these as Cartesian
  // coordinates instead traditional angles because the
  // range of value does not include a wrap around
  u32 pointsInAverage = 0;
  for (const auto& eyeDirection: _eyeDirectionHistory) {
    if (eyeDirection.include) {
      if (filterOutliers) {
        if (eyeDirection.inlier) {
          pointsInAverage += 1;
          averageEyeDirection += eyeDirection.point;
        }
      } else {
        pointsInAverage += 1;
        averageEyeDirection += eyeDirection.point;
      }
    }
  }

  if (pointsInAverage != 0) {
    averageEyeDirection *= 1.f/pointsInAverage;
  } else {
    averageEyeDirection = Point3f(FaceDirectionData3d::kDefaultDistance_cm,
                                  FaceDirectionData3d::kDefaultDistance_cm,
                                  FaceDirectionData3d::kDefaultDistance_cm);
  }
  return averageEyeDirection;
}

Point3f FaceNormalDirectedAtRobot3d::RecomputeEyeDirectionAverage()
{
  return ComputeEyeDirectionAverage(true);
}

int FaceNormalDirectedAtRobot3d::FindEyeDirectionInliers(const Point3f& eyeDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& eyeDirection: _eyeDirectionHistory) {
    /*
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.FindInliers.PointHistory",
                     "direction x=%.3f, y=%.3f, z=%.3f", faceDirection.point.x(),
                     faceDirection.point.y(), faceDirection.point.z());
    */

    if (!eyeDirection.include) {
      continue;
    }
                      
    auto difference = eyeDirection.point - eyeDirectionAverage;
    /*
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.FindInliers.Difference",
                     "direction x=%.3f, y=%.3f, z=%.3f", difference.x(),
                     difference.y(), difference.z());
    */
    if (difference.x() < kInlierXThreshold && difference.y() < kInlierYThreshold && difference.z() < kInlierZThreshold) {
      eyeDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      eyeDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

bool FaceNormalDirectedAtRobot3d::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}

bool FaceNormalDirectedAtRobot3d::GetPointFromHeadPose(const Pose3d& headPose, Point3f& faceDirectionPoint)
{
  Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, 500.f, 0.f)));
  translatedPose.SetParent(headPose);
  auto point = translatedPose.GetWithRespectToRoot().GetTranslation();
  auto translation = headPose.GetTranslation();

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.Translation",
                   "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.SecondPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());

  float alpha = ( -point.z() ) / ( translation.z() - point.z() );

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.Alpha",
                   "alpha: %.3f", alpha);

  auto groundPlanePoint = translation * alpha + point * (1 - alpha);

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.GroundPlanePoint",
                   "x: %.3f, y:%.3f, z:%.3f", groundPlanePoint.x(), groundPlanePoint.y(),
                   groundPlanePoint.z());


  // Alpha less than one means that the ground plane point we found is either
  // in the line segment between our two points (0 < alpha < 1) or in the ray
  // starting at the first point and extending in the same direction as the segment.
  // This avoids finding a intersection with the ground plane when face normal is
  // directed above the horizon.
  if (alpha > 1) {
    faceDirectionPoint = groundPlanePoint;
    return true;
  } else {
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.FaceRayAboveHorizon", "");
    faceDirectionPoint = point;
    return false;
  }
}

bool FaceNormalDirectedAtRobot3d::GetPointFromEyePose(const Pose3d& eyePose, Point3f& eyeDirectionPoint)
{
  // TODO is this too far and do we want to put this on the ground plane?
  Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, -1000.f, 0.f)));
  translatedPose.SetParent(eyePose);
  auto point = translatedPose.GetWithRespectToRoot().GetTranslation();
  auto translation = eyePose.GetTranslation();

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromEyePose.Translation",
                   "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromEyePose.SecondPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());

  eyeDirectionPoint = point;
  return true;
  /*
  float alpha = ( -point.z() ) / ( translation.z() - point.z() );

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.Alpha",
                   "alpha: %.3f", alpha);

  auto groundPlanePoint = translation * alpha + point * (1 - alpha);

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.GroundPlanePoint",
                   "x: %.3f, y:%.3f, z:%.3f", groundPlanePoint.x(), groundPlanePoint.y(),
                   groundPlanePoint.z());


  // Alpha less than one means that the ground plane point we found is either
  // in the line segment between our two points (0 < alpha < 1) or in the ray
  // starting at the first point and extending in the same direction as the segment.
  // This avoids finding a intersection with the ground plane when face normal is
  // directed above the horizon.
  if (alpha > 1) {
    faceDirectionPoint = groundPlanePoint;
    return true;
  } else {
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.FaceRayAboveHorizon", "");
    faceDirectionPoint = point;
    return false;
  }
  */
}

bool FaceNormalDirectedAtRobot3d::IsFaceFocused() const
{
  // TODO for some reason there isn't the right number of inliers
  // so i'm just going to make this 2
  // return ( _numberOfInliers > kHistorySize *.6);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.IsFaceFocused.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  return ( (_numberOfInliers > kMinNumberOfInliers) && _initialized );
}

Point3f FaceNormalDirectedAtRobot3d::GetFaceDirectionSurfaceAverage() const
{
  auto shiftedFaceAverage = _faceDirectionSurfaceAverage + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetFaceDirectionAverage.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", shiftedFaceAverage.x(), shiftedFaceAverage.y(),
                   shiftedFaceAverage.z());
  return shiftedFaceAverage;
}

Point3f FaceNormalDirectedAtRobot3d::GetCurrentFaceDirectionSurface() const
{
  auto shiftedFaceDirection = _faceDirectionSurfaceHistory[_currentIndex].point + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetCurrentFaceDirection.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", shiftedFaceDirection.x(), shiftedFaceDirection.y(),
                   shiftedFaceDirection.z());
  return shiftedFaceDirection;
}

Point3f FaceNormalDirectedAtRobot3d::GetEyeDirectionAverage() const
{
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.EyeDirectionAverage.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", _eyeDirectionAverage.x(), _eyeDirectionAverage.y(), 
                   _eyeDirectionAverage.z());
  return _eyeDirectionAverage;
}

Point3f FaceNormalDirectedAtRobot3d::GetCurrentEyeDirection() const
{
  
  const auto point = _eyeDirectionHistory[_currentIndex].point;
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.CurrentEyeDirection.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());
  return point;
}

} // namespace Vision
} // namespace Anki
