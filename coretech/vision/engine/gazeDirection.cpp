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
  CONSOLE_VAR(u32, kGazeDirectionHistorySize,                 "Vision.GazeDirection",  6);
  CONSOLE_VAR(u32, kGazeDirectionMinNumberOfInliers,          "Vision.GazeDirection",  2);
  CONSOLE_VAR(u32, kGazeDirectionExpireThreshold_ms,          "Vision.GazeDirection",  1000);
  CONSOLE_VAR(f32, kGazeDirectionInlierXThreshold_mm,         "Vision.GazeDirection",  300.f);
  CONSOLE_VAR(f32, kGazeDirectionInlierYThreshold_mm,         "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kGazeDirectionInlierZThreshold_mm,         "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32, kGazeDirectionShiftOutputPointX_mm,        "Vision.GazeDirection",  100.f);
  // This value was chosen to be sufficiently large that the difference in the z coordinates
  // of the two points used to find the intersection with the ground plane weren't too close
  // as to cause numerical instabilities. 500 was too small.
  CONSOLE_VAR(f32, kGazeDirectionSecondPointTranslationY_mm,  "Vision.GazeDirection",  1500.f);
}

GazeDirection::GazeDirection()
  : _gazeDirectionAverage(Point3f(GazeDirectionData::kDefaultDistance_mm,
                                  GazeDirectionData::kDefaultDistance_mm,
                                  GazeDirectionData::kDefaultDistance_mm))
{
  _gazeDirectionHistory.resize(kGazeDirectionHistorySize);
}

void GazeDirection::Update(const TrackedFace& face)
{
  _headPose = face.GetHeadPose();
  _lastUpdated = face.GetTimeStamp();

  if (_currentIndex > (kGazeDirectionHistorySize - 1)) {
    // Make sure we place a "real" value
    // in every element of the history
    // before returning a result
    _initialized = true;
    _currentIndex = 0;
  }

  Point3f gazeDirectionPoint;
  const bool include = GetPointFromHeadPose(_headPose, gazeDirectionPoint);
  _gazeDirectionHistory[_currentIndex].Update(gazeDirectionPoint, include);

  // This computations only need to happen once we are initialized because
  // otherwise we're not going to output a point as stable
  if (_initialized) {
    _gazeDirectionAverage = ComputeEntireGazeDirectionAverage();
    _numberOfInliers = FindInliers(_gazeDirectionAverage);
    _gazeDirectionAverage = RecomputeGazeDirectionAverage();
  }

  _currentIndex++;
}

Point3f GazeDirection::ComputeEntireGazeDirectionAverage()
{
  return ComputeGazeDirectionAverage(false);
}

Point3f GazeDirection::ComputeGazeDirectionAverage(const bool filterOutliers)
{
  Point3f averageGazeDirection = Point3f(0.f, 0.f, 0.f);

  // Find the average, these are Cartesian
  // coordinates. However we only want to include
  // those coordinates that intersected the ground plane.
  u32 pointsInAverage = 0;
  for (const auto& gazeDirection: _gazeDirectionHistory) {
    if (gazeDirection.include) {
      if (filterOutliers) {
        if (gazeDirection.inlier) {
          pointsInAverage++;
          averageGazeDirection += gazeDirection.point;
        }
      } else {
        pointsInAverage++;
        averageGazeDirection += gazeDirection.point;
      }
    }
  }

  if (pointsInAverage != 0) {
    averageGazeDirection *= 1.f/pointsInAverage;
  } else {
    averageGazeDirection = Point3f(GazeDirectionData::kDefaultDistance_mm,
                                   GazeDirectionData::kDefaultDistance_mm,
                                   GazeDirectionData::kDefaultDistance_mm);
  }
  return averageGazeDirection;
}

int GazeDirection::FindInliers(const Point3f& gazeDirectionAverage)
{
  // Find the inliers given the average, and only include
  // points that have intersected with the ground plane.
  // Here inliners are determined using a l-1 distance.
  int numberOfInliers = 0;
  for (auto& gazeDirection: _gazeDirectionHistory) {

    if (!gazeDirection.include) {
      continue;
    }
                      
    auto difference = gazeDirection.point - gazeDirectionAverage;
    if (std::abs(difference.x()) < kGazeDirectionInlierXThreshold_mm &&
        std::abs(difference.y()) < kGazeDirectionInlierYThreshold_mm &&
        std::abs(difference.z()) < kGazeDirectionInlierZThreshold_mm) {
      gazeDirection.inlier = true;
      numberOfInliers++;
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
  return (currentTime - _lastUpdated) > kGazeDirectionExpireThreshold_ms;
}

bool GazeDirection::GetPointFromHeadPose(const Pose3d& headPose, Point3f& gazeDirectionPoint)
{
  // Get another point in the direction of the rotation of the head pose, to then find the
  // intersection of that line with ground plane, note that this point is in world coordinates
  // "behind" the head if the pose were looking at the ground plane if the y translation is
  // positive.
  Pose3d translatedPose(0.f, Z_AXIS_3D(), {0.f, kGazeDirectionSecondPointTranslationY_mm, 0.f}, headPose);
  const auto& point = translatedPose.GetWithRespectToRoot().GetTranslation();
  const auto& translation = headPose.GetTranslation();

  if (Util::IsFltNear(translation.z(), point.z())) {
    gazeDirectionPoint = point;
    return false;
  } else {
    // This is adopting the definition of a line l as the set
    // { P_1 * alpha + P_2 * (1-alpha) | alpha in R}
    // and the points (including the zero vector) are
    // in R^3. P_1 is the position of the head (translation) and P_2 is
    // position of the point in the direction of the head pose's rotation
    // (point).
    const float alpha = ( -point.z() ) / ( translation.z() - point.z() );

    // Alpha less than zero means that the ground plane point we found is "behind"
    // the head pose. For some reason the head pose rotation is such
    // that the face normal is pointed "above" the horizon. If alpha > 0 the point is
    // either in the line segment between our two points (0 < alpha < 1) or in the ray
    // starting at the second point and extending in the same direction as the segment.
    // This avoids finding a intersection with the ground plane when face normal is
    // directed above the horizon.
    if (FLT_GT(alpha, 1.f)) {
      gazeDirectionPoint = translation * alpha + point * (1 - alpha);
      return true;
    } else {
      gazeDirectionPoint = point;
      return false;
    }
  }
}

bool GazeDirection::IsStable() const
{
  return ( (_numberOfInliers > kGazeDirectionMinNumberOfInliers) && _initialized );
}

Point3f GazeDirection::GetGazeDirectionAverage() const
{
  return _gazeDirectionAverage + Point3f(kGazeDirectionShiftOutputPointX_mm, 0.f, 0.f);
}

Point3f GazeDirection::GetCurrentGazeDirection() const
{
  return _gazeDirectionHistory[_currentIndex].point + Point3f(kGazeDirectionShiftOutputPointX_mm, 0.f, 0.f);
}

void GazeDirection::ClearHistory()
{
  _initialized = false;
  for (auto& gazeDirection: _gazeDirectionHistory) {
      gazeDirection.point.x() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.point.y() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.point.z() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.inlier = false;
      gazeDirection.include  = false;
  }
}

} // namespace Vision
} // namespace Anki
