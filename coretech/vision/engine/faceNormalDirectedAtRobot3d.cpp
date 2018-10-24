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
  CONSOLE_VAR(f32, kInlierDistanceSq,                 "Vision.EyeContact",  100.f);
  CONSOLE_VAR(u32, kMinNumberOfInliers,               "Vision.EyeContact",  3);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotDistanceSq,    "Vision.EyeContact",  11.f*11.f);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotYawAbsThres,   "Vision.EyeContact",  10.f);
  CONSOLE_VAR(f32, kFaceDirectedAtRobotPitchAbsThres, "Vision.EyeContact",  25.f);
  CONSOLE_VAR(f32, kFaceDirectedRightYawAbsThres,     "Vision.EyeContact",  15.f);
  CONSOLE_VAR(f32, kFaceDirectedLeftYawAbsThres,      "Vision.EyeContact", -15.f);
  CONSOLE_VAR(u32, kExpireThreshold,                  "Vision.EyeContact",  50);
}

FaceNormalDirectedAtRobot3d::FaceNormalDirectedAtRobot3d()
  : _faceDirectionAverage(Point3f(FaceDirectionData3d::kDefaultDistance_cm,
                                  FaceDirectionData3d::kDefaultDistance_cm,
                                  FaceDirectionData3d::kDefaultDistance_cm))
{
  _faceDirectionHistory.resize(kHistorySize);
}

void FaceNormalDirectedAtRobot3d::Update(const TrackedFace& face,
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

  Point3f faceDirectionPoint;
  bool include = GetPointFromHeadPose(_headPose, faceDirectionPoint);

  /*
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.YawPitch",
                   "Yaw=%.3f, Pitch=%.3f", RAD_TO_DEG(_face.GetHeadYaw().ToFloat()),
                   RAD_TO_DEG(_face.GetHeadPitch().ToFloat()));
  */
  _faceDirectionHistory[_currentIndex].Update(faceDirectionPoint,
                                              RAD_TO_DEG(face.GetHeadPitch().ToFloat()),
                                              RAD_TO_DEG(face.GetHeadYaw().ToFloat()),
                                              include);

  _faceDirectionAverage = ComputeEntireFaceDirectionAverage();
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.AverageYawAndPitch",
                   "average x=%.3f, average y=%.3f, average z=%.3f", _faceDirectionAverage.x(),
                   _faceDirectionAverage.y(), _faceDirectionAverage.z());
  _numberOfInliers = FindInliers(_faceDirectionAverage);
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.NumberOfInliers",
                   "Number of Inliers = %d", _numberOfInliers);
  _faceDirectionAverage = RecomputeFaceDirectionAverage();
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.Update.RecomputedAverageYawAndPitch",
                   "average x=%.3f, average y=%.3f, average z=%.3f", _faceDirectionAverage.x(),
                   _faceDirectionAverage.y(), _faceDirectionAverage.z());
  _faceDirection = DetermineFaceDirection();

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
  for (const auto& faceDirection: _faceDirectionHistory) {
    if (faceDirection.include) {
      if (filterOutliers) {
        pointsInAverage += 1;
        averageFaceDirection += faceDirection.point;
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
  for (auto& faceDirection: _faceDirectionHistory) {
    PRINT_NAMED_DEBUG("FaceNormalDirectedAtRobot3d.Update.FindInliers",
                      "direction x=%.3f, y=%.3f, z=%.3f", faceDirection.point.x(),
                      faceDirection.point.y(), faceDirection.point.z());
                      
    auto difference = faceDirection.point - faceDirectionAverage;
    if (difference.x() < 300 && difference.y() < 100 && difference.z() < 20) {
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

TrackedFace::FaceDirection FaceNormalDirectedAtRobot3d::DetermineFaceDirection()
{
  if (_numberOfInliers > kMinNumberOfInliers) {
    bool withinDistance = std::abs(_faceDirectionAverage.x()) < kFaceDirectedAtRobotYawAbsThres;
    withinDistance &= std::abs(_faceDirectionAverage.x()) < kFaceDirectedAtRobotPitchAbsThres;
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.DetermineFaceDirection.Distance",
                     "threshold yaw=%.3f, threshold pitch=%.3f",
                     kFaceDirectedAtRobotYawAbsThres, kFaceDirectedAtRobotPitchAbsThres);
    if (withinDistance) {
      return TrackedFace::FaceDirection::AtRobot;
    }
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.DetermineFaceDirection.Threshold",
                     "threshold yaw=%.3f", kFaceDirectedRightYawAbsThres);
    if (_faceDirectionAverage.x() > kFaceDirectedRightYawAbsThres) {
      return TrackedFace::FaceDirection::RightOfRobot;
    }
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.DetermineFaceDirection.Threshold",
                     "threshold yaw=%.3f", kFaceDirectedLeftYawAbsThres);
    if (_faceDirectionAverage.x() < kFaceDirectedLeftYawAbsThres) {
      return TrackedFace::FaceDirection::LeftOfRobot;
    }
  }

  return TrackedFace::FaceDirection::None;
}

bool FaceNormalDirectedAtRobot3d::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}

bool FaceNormalDirectedAtRobot3d::GetPointFromHeadPose(const Pose3d& headPose, Point3f& faceDirectionPoint)
{
  // TODO look over history and determine if it's above or below the horizon
  bool aboveHorizon = IsFaceDirectionAboveHorizon();

  // TODO here is where I should determine whether to do ground plane or
  // follow the ray and look for a face
  /*
  auto rotationX = headPose.GetRotationAngle<'X'>();
  auto rotationY = headPose.GetRotationAngle<'Y'>();
  auto rotationZ = headPose.GetRotationAngle<'Z'>();
  auto quatnerion = headPose.GetRotation().GetQuaternion();
  auto w = quatnerion.w();
  auto x = quatnerion.x();
  auto y = quatnerion.y();
  auto z = quatnerion.z();
  */

  // I think all of these should be cosine's but this should probably
  // be double checked.
  if (aboveHorizon) {
    auto rotationMatrix = headPose.GetRotationMatrix();
    auto faceRay = rotationMatrix * Vec3f(1200.f, 0.f, 0.f);
    PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.DirectionAboveHorizon",
                     "x: %.3f, y:%.3f, z:%.3f", faceRay.x(), faceRay.y(), faceRay.z());
    // determine units of translation ... probably in mm or cm
    faceDirectionPoint = headPose.GetTranslation() + faceRay;
  } else {
    auto rotationMatrix = headPose.GetRotationMatrix();
    auto faceRay = rotationMatrix * Vec3f(1.f, 0.f, 0.f);
    if (faceRay.z() < 0) {
      PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.FaceRay",
                       "x: %.3f, y:%.3f, z:%.3f", faceRay.x(), faceRay.y(), faceRay.z());
      if (!NEAR_ZERO(faceRay.z())) {
        float distance = headPose.GetTranslation().z() / -faceRay.z();
        PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.Distance",
                         "distance: %.3f", distance);
        Point3f a = headPose.GetTranslation();
        Point3f b = a + (rotationMatrix * Vec3f(distance, 0.f, 0.f));
        PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.FaceTranslation",
                         "x: %.3f, y:%.3f, z:%.3f", a.x(), a.y(), a.z());
        PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.SecondPoint",
                         "x: %.3f, y:%.3f, z:%.3f", b.x(), b.y(), b.z());
        /*
        auto groundPoint = IntersectionDirectionWithGroundPlane(a, b, distance);
        PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.GetPointFromHeadPose.DirectionBelowHorizon",
                         "x: %.3f, y:%.3f, z:%.3f", groundPoint.x(), groundPoint.y(), groundPoint.z());
        */
        faceDirectionPoint = b; 
        return true;
      }
    } else {
      // Can't do anything there won't be an intersection
      return false;
    }
  }
  return false;
}

Point3f FaceNormalDirectedAtRobot3d::IntersectionDirectionWithGroundPlane(const Point3f& a,
                                                                          const Point3f b,
                                                                          const float distance)
{
  Vec3f groundPlaneNormal(0.f, 0.f, 1.f);
  double numerator = ((groundPlaneNormal.x() * a.x()) + (groundPlaneNormal.y() * a.y()) + (groundPlaneNormal.z() * a.z()));
  double denominator = ((groundPlaneNormal.x()* (a.x() - b.x())) + (groundPlaneNormal.y() * (a.y() - b.y()))
                  + (groundPlaneNormal.z() * (a.z() - b.z())));
  /*
  double u = ((groundPlaneNormal.x() * a.x()) + (groundPlaneNormal.y() * a.y()) + (groundPlaneNormal.z() * a.z()))
               / ((groundPlaneNormal.x()* (a.x() - b.x())) + (groundPlaneNormal.y() * (a.y() - b.y()))
                  + (groundPlaneNormal.z() * (a.z() - b.z())));
  */
  double u = numerator / denominator;
  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.IntersectionDirectionWithGroundPlane.Intermediate",
                   "numerator: %.3f denominator: %.3f u: %.3f", numerator, denominator, u);

  // TODO i am pretty sure this part is wrong
  return Point3f(
    u * a.x() + (1-u) * b.x(),
    u * a.y() + (1-u) * b.y(),
    u * a.z() + (1-u) * b.z()
  );
}

bool FaceNormalDirectedAtRobot3d::IsFaceDirectionAboveHorizon()
{
  // TODO look over the history of head poses and determine
  // whether we think the face is directed above or below the horizon
  // which will influence whether we try to interesect the direction
  // with a specific distance of the ground plane

  Point2f averageFaceDirection = Point2f(0.f, 0.f);
  auto numberInAverage = 0;
  for (auto & entry: _faceDirectionHistory)
  {
    averageFaceDirection += entry.angles;
  }
  if (numberInAverage != 0) {
    averageFaceDirection /= numberInAverage;
  }

  PRINT_NAMED_INFO("FaceNormalDirectedAtRobot3d.IsFaceDirectionAboveHorizon.AverageFaceAngles",
                   "yaw: %.3f pitch: %.3f", averageFaceDirection.x(), averageFaceDirection.y());

  return false;
}

} // namespace Vision
} // namespace Anki
