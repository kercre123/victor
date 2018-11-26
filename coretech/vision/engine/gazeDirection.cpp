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
#include "gazeDirection.h"

#include "coretech/common/shared/types.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR(u32, kHistorySize,                      "Vision.GazeDirection",  6);
  CONSOLE_VAR(f32, kInlierDistanceSq,                 "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(u32, kMinNumberOfInliers,               "Vision.GazeDirection",  2);
  CONSOLE_VAR(u32, kExpireThreshold,                  "Vision.GazeDirection",  50);
  CONSOLE_VAR(f32, kInlierXThreshold,                 "Vision.GazeDirection",  300.f);
  CONSOLE_VAR(f32, kInlierYThreshold,                 "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kInlierZThreshold,                 "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32, kShiftOutputPointX_mm,             "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kGazeAgreementX,                   "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kGazeAgreementY,                   "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kGazeAgreementZ,                   "Vision.GazeDirection",  100.f);
}

GazeDirection::GazeDirection()
  : _faceDirectionSurfaceAverage(Point3f(GazeDirectionData::kDefaultDistance_cm,
                                 GazeDirectionData::kDefaultDistance_cm,
                                 GazeDirectionData::kDefaultDistance_cm)),
  _eyeDirectionAverage(Point3f(GazeDirectionData::kDefaultDistance_cm,
                               GazeDirectionData::kDefaultDistance_cm,
                               GazeDirectionData::kDefaultDistance_cm))
{
  _faceDirectionSurfaceHistory.resize(kHistorySize);
  _eyeDirectionHistory.resize(kHistorySize);
}

void GazeDirection::Update(const TrackedFace& face,
                                         const TimeStamp_t timeStamp)
{
  _headPose = face.GetHeadPose();
  _eyePose = face.GetEyePose();
  _eyePose.Print("GazeDirection.Update.EyePose");
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

  PRINT_NAMED_INFO("GazeDirection.Update.FaceAngles",
                   "yaw=%.3f, pitch=%.3f, roll=%.3f", RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                   RAD_TO_DEG(face.GetHeadPitch().ToFloat()), RAD_TO_DEG(face.GetHeadRoll().ToFloat()));

  Point3f faceDirectionPoint;
  bool include = GetPointFromHeadPose(_headPose, faceDirectionPoint);
  _faceDirectionSurfaceHistory[_currentIndex].Update(faceDirectionPoint,
                                              RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                                              RAD_TO_DEG(face.GetHeadPitch().ToFloat()),
                                              include);

  _faceDirectionSurfaceAverage = ComputeEntireFaceDirectionAverage();
  _numberOfInliers = FindInliers(_faceDirectionSurfaceAverage);
  PRINT_NAMED_INFO("GazeDirection.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _faceDirectionSurfaceAverage = RecomputeFaceDirectionAverage();

  Point3f eyeDirectionPoint;
  include = GetPointFromEyePose(_eyePose, eyeDirectionPoint);
  PRINT_NAMED_INFO("GazeDirection.Update.eyeDirectionPoint",
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
  PRINT_NAMED_INFO("GazeDirection.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _eyeDirectionAverage = RecomputeEyeDirectionAverage();

  _currentIndex += 1;
}

Point3f GazeDirection::ComputeEntireFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(false);
}

Point3f GazeDirection::ComputeFaceDirectionAverage(const bool filterOutliers)
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
    averageFaceDirection = Point3f(GazeDirectionData::kDefaultDistance_cm,
                                   GazeDirectionData::kDefaultDistance_cm,
                                   GazeDirectionData::kDefaultDistance_cm);
  }
  return averageFaceDirection;
}

int GazeDirection::FindInliers(const Point3f& faceDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& faceDirection: _faceDirectionSurfaceHistory) {

    if (!faceDirection.include) {
      continue;
    }
                      
    auto difference = faceDirection.point - faceDirectionAverage;
    if (difference.x() < kInlierXThreshold && difference.y() < kInlierYThreshold && difference.z() < kInlierZThreshold) {
      faceDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      faceDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

Point3f GazeDirection::RecomputeFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(true);
}

Point3f GazeDirection::ComputeEntireEyeDirectionAverage()
{
  return ComputeEyeDirectionAverage(false);
}

Point3f GazeDirection::ComputeEyeDirectionAverage(const bool filterOutliers)
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
    averageEyeDirection = Point3f(GazeDirectionData::kDefaultDistance_cm,
                                  GazeDirectionData::kDefaultDistance_cm,
                                  GazeDirectionData::kDefaultDistance_cm);
  }
  return averageEyeDirection;
}

Point3f GazeDirection::RecomputeEyeDirectionAverage()
{
  return ComputeEyeDirectionAverage(true);
}

int GazeDirection::FindEyeDirectionInliers(const Point3f& eyeDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& eyeDirection: _eyeDirectionHistory) {

    if (!eyeDirection.include) {
      continue;
    }
                      
    auto difference = eyeDirection.point - eyeDirectionAverage;
    if (difference.x() < kInlierXThreshold && difference.y() < kInlierYThreshold && difference.z() < kInlierZThreshold) {
      eyeDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      eyeDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

bool GazeDirection::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}

bool GazeDirection::GetPointFromHeadPose(const Pose3d& headPose, Point3f& faceDirectionPoint)
{
  Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, 500.f, 0.f)));
  translatedPose.SetParent(headPose);
  auto point = translatedPose.GetWithRespectToRoot().GetTranslation();
  auto translation = headPose.GetTranslation();

  PRINT_NAMED_INFO("GazeDirection.GetPointFromHeadPose.Translation",
                   "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
  PRINT_NAMED_INFO("GazeDirection.GetPointFromHeadPose.SecondPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());

  float alpha = ( -point.z() ) / ( translation.z() - point.z() );

  PRINT_NAMED_INFO("GazeDirection.GetPointFromHeadPose.Alpha",
                   "alpha: %.3f", alpha);

  auto groundPlanePoint = translation * alpha + point * (1 - alpha);

  PRINT_NAMED_INFO("GazeDirection.GetPointFromHeadPose.GroundPlanePoint",
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
    PRINT_NAMED_INFO("GazeDirection.GetPointFromHeadPose.FaceRayAboveHorizon", "");
    faceDirectionPoint = point;
    return false;
  }
}

bool GazeDirection::GetPointFromEyePose(const Pose3d& eyePose, Point3f& eyeDirectionPoint)
{
  // TODO is this too far and do we want to put this on the ground plane?
  // Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, -1000.f, 0.f)));
  Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, 500.f, 0.f)));
  translatedPose.SetParent(eyePose);
  auto point = translatedPose.GetWithRespectToRoot().GetTranslation();
  auto translation = eyePose.GetTranslation();

  PRINT_NAMED_INFO("GazeDirection.GetPointFromEyePose.Translation",
                   "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
  PRINT_NAMED_INFO("GazeDirection.GetPointFromEyePose.SecondPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());

  float alpha = ( -point.z() ) / ( translation.z() - point.z() );

  PRINT_NAMED_INFO("GazeDirection.GetPointFromEyePose.Alpha",
                   "alpha: %.3f", alpha);

  auto groundPlanePoint = translation * alpha + point * (1 - alpha);

  PRINT_NAMED_INFO("GazeDirection.GetPointFromEyePose.GroundPlanePoint",
                   "x: %.3f, y:%.3f, z:%.3f", groundPlanePoint.x(), groundPlanePoint.y(),
                   groundPlanePoint.z());


  // Alpha less than one means that the ground plane point we found is either
  // in the line segment between our two points (0 < alpha < 1) or in the ray
  // starting at the first point and extending in the same direction as the segment.
  // This avoids finding a intersection with the ground plane when face normal is
  // directed above the horizon.
  if (alpha > 1) {
    eyeDirectionPoint = groundPlanePoint;
    return true;
  } else {
    PRINT_NAMED_INFO("GazeDirection.GetPointEyePose.EyeRayAboveHorizon", "");
    eyeDirectionPoint = point;
    return false;
  }
}

bool GazeDirection::IsFaceFocused() const
{
  // TODO for some reason there isn't the right number of inliers
  // so i'm just going to make this 2
  // return ( _numberOfInliers > kHistorySize *.6);
  PRINT_NAMED_INFO("GazeDirection.IsFaceFocused.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  return ( (_numberOfInliers > kMinNumberOfInliers) && _initialized );
}

Point3f GazeDirection::GetFaceDirectionSurfaceAverage() const
{
  auto shiftedFaceAverage = _faceDirectionSurfaceAverage + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  PRINT_NAMED_INFO("GazeDirection.GetFaceDirectionAverage.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", shiftedFaceAverage.x(), shiftedFaceAverage.y(),
                   shiftedFaceAverage.z());
  return shiftedFaceAverage;
}

Point3f GazeDirection::GetCurrentFaceDirectionSurface() const
{
  auto shiftedFaceDirection = _faceDirectionSurfaceHistory[_currentIndex].point + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  PRINT_NAMED_INFO("GazeDirection.GetCurrentFaceDirection.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", shiftedFaceDirection.x(), shiftedFaceDirection.y(),
                   shiftedFaceDirection.z());
  return shiftedFaceDirection;
}

Point3f GazeDirection::GetEyeDirectionAverage() const
{
  PRINT_NAMED_INFO("GazeDirection.EyeDirectionAverage.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", _eyeDirectionAverage.x(), _eyeDirectionAverage.y(), 
                   _eyeDirectionAverage.z());
  return _eyeDirectionAverage;
}

Point3f GazeDirection::GetCurrentEyeDirection() const
{
  
  const auto point = _eyeDirectionHistory[_currentIndex].point;
  PRINT_NAMED_INFO("GazeDirection.CurrentEyeDirection.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", point.x(), point.y(), point.z());
  return point;
}

void GazeDirection::DoEyeAndHeadAgree(const Point3f& headPointAverage,
                                                    const Point3f& eyePointAverage)
{
  // If things are close enough say they agree ... does directtionality matter?
  auto difference = headPointAverage - eyePointAverage;
  if (difference.x() < kGazeAgreementX && difference.y() < kGazeAgreementY &&
      difference.z() < kGazeAgreementZ) {
    _gazeAgreement = true;
  }
}

Point3f GazeDirection::GetTotalGazeAvergae() const
{
  if (_gazeAgreement) {
    return (GetEyeDirectionAverage() + GetFaceDirectionSurfaceAverage()) * .5f;
  } else {
    return GetFaceDirectionSurfaceAverage();
  }
}

void GazeDirection::ClearHistory() {
  _initialized = false;
  for (auto& faceDirection: _faceDirectionSurfaceHistory) {
      faceDirection.point = Point3f(GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm);
      faceDirection.inlier = false;
      faceDirection.include  = false;
      faceDirection.angles = Point2f(GazeDirectionData::kYawMin_deg, GazeDirectionData::kPitchMin_deg);
  }
  for (auto& eyeDirection: _eyeDirectionHistory) {
      eyeDirection.point = Point3f(GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm);
      eyeDirection.inlier = false;
      eyeDirection.include  = false;
      eyeDirection.angles = Point2f(GazeDirectionData::kYawMin_deg, GazeDirectionData::kPitchMin_deg);
  }
}


} // namespace Vision
} // namespace Anki
