/**
 * File: gazeDirection.cpp
 *
 * Author: Robert Cosgriff
 * Date:   11/26/2018
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
  : _gazeDirectionAverage(Point3f(GazeDirectionData::kDefaultDistance_cm,
                                 GazeDirectionData::kDefaultDistance_cm,
                                 GazeDirectionData::kDefaultDistance_cm))
{
  _gazeDirectionHistory.resize(kHistorySize);
}

void GazeDirection::Update(const TrackedFace& face,
                                         const TimeStamp_t timeStamp)
{
  _headPose = face.GetHeadPose();
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

  Point3f gazeDirectionPoint;
  bool include = GetPointFromHeadPose(_headPose, gazeDirectionPoint);
  _gazeDirectionHistory[_currentIndex].Update(gazeDirectionPoint,
                                              RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                                              RAD_TO_DEG(face.GetHeadPitch().ToFloat()),
                                              include);

  _gazeDirectionAverage = ComputeEntireGazeDirectionAverage();
  _numberOfInliers = FindInliers(_gazeDirectionAverage);
  _gazeDirectionAverage = RecomputeGazeDirectionAverage();

  _currentIndex += 1;
}

Point3f GazeDirection::ComputeEntireGazeDirectionAverage()
{
  return ComputeGazeDirectionAverage(false);
}

Point3f GazeDirection::ComputeGazeDirectionAverage(const bool filterOutliers)
{
  Point3f averageGazeDirection = Point3f(0.f, 0.f, 0.f);

  // Find the average, we can treat these as Cartesian
  // coordinates instead traditional angles because the
  // range of value does not include a wrap around
  u32 pointsInAverage = 0;
  for (const auto& gazeDirection: _gazeDirectionHistory) {
    if (gazeDirection.include) {
      if (filterOutliers) {
        if (gazeDirection.inlier) {
          pointsInAverage += 1;
          averageGazeDirection += gazeDirection.point;
        }
      } else {
        pointsInAverage += 1;
        averageGazeDirection += gazeDirection.point;
      }
    }
  }

  if (pointsInAverage != 0) {
    averageGazeDirection *= 1.f/pointsInAverage;
  } else {
    averageGazeDirection = Point3f(GazeDirectionData::kDefaultDistance_cm,
                                   GazeDirectionData::kDefaultDistance_cm,
                                   GazeDirectionData::kDefaultDistance_cm);
  }
  return averageGazeDirection;
}

int GazeDirection::FindInliers(const Point3f& gazeDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& gazeDirection: _gazeDirectionHistory) {

    if (!gazeDirection.include) {
      continue;
    }
                      
    auto difference = gazeDirection.point - gazeDirectionAverage;
    if (difference.x() < kInlierXThreshold && difference.y() < kInlierYThreshold && difference.z() < kInlierZThreshold) {
      gazeDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      gazeDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

Point3f GazeDirection::RecomputeGazeDirectionAverage()
{
  return ComputeGazeDirectionAverage(true);
}

bool GazeDirection::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}

bool GazeDirection::GetPointFromHeadPose(const Pose3d& headPose, Point3f& gazeDirectionPoint)
{
  Pose3d translatedPose = Pose3d(Transform3d(Rotation3d(0.f, Z_AXIS_3D()), Point3f(0.f, 500.f, 0.f)));
  translatedPose.SetParent(headPose);
  auto point = translatedPose.GetWithRespectToRoot().GetTranslation();
  auto translation = headPose.GetTranslation();

  float alpha = ( -point.z() ) / ( translation.z() - point.z() );
  auto groundPlanePoint = translation * alpha + point * (1 - alpha);

  // Alpha less than one means that the ground plane point we found is either
  // in the line segment between our two points (0 < alpha < 1) or in the ray
  // starting at the first point and extending in the same direction as the segment.
  // This avoids finding a intersection with the ground plane when face normal is
  // directed above the horizon.
  if (alpha > 1) {
    gazeDirectionPoint = groundPlanePoint;
    return true;
  } else {
    gazeDirectionPoint = point;
    return false;
  }
}

bool GazeDirection::IsStable() const
{
  // TODO for some reason there isn't the right number of inliers
  // so i'm just going to make this 2
  // return ( _numberOfInliers > kHistorySize *.6);
  PRINT_NAMED_INFO("GazeDirection.IsStable.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  return ( (_numberOfInliers > kMinNumberOfInliers) && _initialized );
}

Point3f GazeDirection::GetGazeDirectionAverage() const
{
  auto shiftedFaceAverage = _gazeDirectionAverage + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  PRINT_NAMED_INFO("GazeDirection.GetGazeDirectionAverage.ReturnedPoint",
                   "x: %.3f, y:%.3f, z:%.3f", shiftedFaceAverage.x(), shiftedFaceAverage.y(),
                   shiftedFaceAverage.z());
  return shiftedFaceAverage;
}

Point3f GazeDirection::GetCurrentGazeDirection() const
{
  auto shiftedGazeDirection = _gazeDirectionHistory[_currentIndex].point + Point3f(kShiftOutputPointX_mm, 0.f, 0.f);
  return shiftedGazeDirection;
}

void GazeDirection::ClearHistory() {
  _initialized = false;
  for (auto& gazeDirection: _gazeDirectionHistory) {
      gazeDirection.point = Point3f(GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm, GazeDirectionData::kDefaultDistance_cm);
      gazeDirection.inlier = false;
      gazeDirection.include  = false;
      gazeDirection.angles = Point2f(GazeDirectionData::kYawMin_deg, GazeDirectionData::kPitchMin_deg);
  }
}

} // namespace Vision
} // namespace Anki
