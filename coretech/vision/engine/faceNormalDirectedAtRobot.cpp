
/**
 * File: faceNormalDirectedAtRobot.cpp
 *
 * Author: Robert Cosgriff
 * Date:   9/5/2018
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 **/
#include "faceNormalDirectedAtRobot.h"

#include "coretech/common/shared/types.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR(u32, kHistorySize,                      "Vision.FaceNormalDirectedAtRobot",  6);
  CONSOLE_VAR(f32, kInlierDistanceSq,                 "Vision.EyeContact",  100.f);
  CONSOLE_VAR(u32, kMinNumberOfInliers,               "Vision.EyeContact",  3);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotDistanceSq,    "Vision.EyeContact",  11.f*11.f);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotYawAbsThres,   "Vision.EyeContact",  10.f);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotPitchAbsThres, "Vision.EyeContact",  25.f);
  CONSOLE_VAR(f32, kFaceDirectedRightYawAbsThres,     "Vision.EyeContact",  15.f);
  CONSOLE_VAR(f32, kFaceDirectedLeftYawAbsThres,      "Vision.EyeContact", -15.f);
  CONSOLE_VAR(u32, kExpireThreshold,                  "Vision.EyeContact",  50);
}

FaceNormalDirectedAtRobot::FaceNormalDirectedAtRobot()
  : _faceDirectionAverage(Point2f(FaceDirectionData::kYawMin_deg,
                                  FaceDirectionData::kPitchMin_deg))
{
  _faceDirectionHistory.resize(kHistorySize);
}

void FaceNormalDirectedAtRobot::Update(const TrackedFace& face,
                                       const TimeStamp_t timeStamp)
{
  _face = face;
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


  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.Update.YawPitch",
                   "Yaw=%.3f, Pitch=%.3f", RAD_TO_DEG(_face.GetHeadYaw().ToFloat()),
                   RAD_TO_DEG(_face.GetHeadPitch().ToFloat()));
  _faceDirectionHistory[_currentIndex].Update(
    Point2f(
      RAD_TO_DEG(_face.GetHeadYaw().ToFloat()),
      RAD_TO_DEG(_face.GetHeadPitch().ToFloat())
    )
  );

  _faceDirectionAverage = ComputeEntireFaceDirectionAverage();
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.Update.AverageYawAndPitch",
                   "average yaw=%.3f, average pitch=%.3f", _faceDirectionAverage.x(),
                   _faceDirectionAverage.y());
  _numberOfInliers = FindInliers(_faceDirectionAverage);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _faceDirectionAverage = RecomputeFaceDirectionAverage();
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.Update.RecomputedAverageYawAndPitch",
                   "average yaw=%.3f, average pitch=%.3f", _faceDirectionAverage.x(),
                   _faceDirectionAverage.y());
  _faceDirection = DetermineFaceDirection();

  _currentIndex += 1;
}

Point2f FaceNormalDirectedAtRobot::ComputeEntireFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(false);
}

Point2f FaceNormalDirectedAtRobot::ComputeFaceDirectionAverage(const bool filterOutliers)
{
  Point2f averageFaceDirection = Point2f(0.f, 0.f);

  // Find the average, we can treat these as Cartesian
  // coordinates instead traditional angles because the
  // range of value does not include a wrap around
  u32 pointsInAverage = 0;
  for (const auto& faceDirection: _faceDirectionHistory) {
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

  if (pointsInAverage != 0) {
    averageFaceDirection *= 1.f/pointsInAverage;
  } else {
    averageFaceDirection = Point2f(FaceDirectionData::kYawMin_deg, FaceDirectionData::kPitchMin_deg);
  }
  return averageFaceDirection;
}

int FaceNormalDirectedAtRobot::FindInliers(const Point2f& faceDirectionAverage)
{
  int numberOfInliers = 0;
  for (auto& faceDirection: _faceDirectionHistory) {
    auto difference = faceDirection.point - faceDirectionAverage;
    auto distance = difference.LengthSq();
    if (distance < kInlierDistanceSq) {
      faceDirection.inlier = true;
      numberOfInliers += 1;
    } else {
      faceDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

Point2f FaceNormalDirectedAtRobot::RecomputeFaceDirectionAverage()
{
  return ComputeFaceDirectionAverage(true);
}

TrackedFace::FaceDirection FaceNormalDirectedAtRobot::DetermineFaceDirection()
{
  if (_numberOfInliers > kMinNumberOfInliers) {
    bool withinDistance = std::abs(_faceDirectionAverage.x()) < kFaceDirectedAtRobotYawAbsThres;
    withinDistance &= std::abs(_faceDirectionAverage.x()) < kFaceDirectedAtRobotPitchAbsThres;
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.DetermineFaceDirection.Distance",
                     "threshold yaw=%.3f, threshold pitch=%.3f",
                     kFaceDirectedAtRobotYawAbsThres, kFaceDirectedAtRobotPitchAbsThres);
    if (withinDistance) {
      return TrackedFace::FaceDirection::AtRobot;
    }
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.DetermineFaceDirection.Threshold",
                     "threshold yaw=%.3f", kFaceDirectedRightYawAbsThres);
    if (_faceDirectionAverage.x() > kFaceDirectedRightYawAbsThres) {
      return TrackedFace::FaceDirection::RightOfRobot;
    }
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot.DetermineFaceDirection.Threshold",
                     "threshold yaw=%.3f", kFaceDirectedLeftYawAbsThres);
    if (_faceDirectionAverage.x() < kFaceDirectedLeftYawAbsThres) {
      return TrackedFace::FaceDirection::LeftOfRobot;
    }
  }

  return TrackedFace::FaceDirection::None;
}

bool FaceNormalDirectedAtRobot::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}

} // namespace Vision
} // namespace Anki
