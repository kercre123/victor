/**
 * File: faceNormalDirectedAtRobot3d.h
 *
 * Author: Robert Cosgriff
 * Date:   9/5/2018
 *
 * Description: Class that determines if a face normal is directed at the robot.
 *              In plain language, whether a person's head is "facing" the robot.
 *              However instead of using angles that are determined from the face
 *              normal this class uses the full 3D pose of the face. For the purpose
 *              of determining whether to look for an object on the ground plane
 *              that the face might be looking at, or if the face is directed
 *              in a manner that would indicate we should look for a face.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Vision_FaceNormalDirectedAtRobot3D_H__
#define __Anki_Vision_FaceNormalDirectedAtRobot3D_H__

#include "coretech/vision/engine/trackedFace.h"

namespace Anki {
namespace Vision {

struct FaceDirectionData3d
{
  constexpr static const float kDefaultDistance_cm = -100000.f;
  constexpr static const float kYawMin_deg      = -180.f;
  constexpr static const float kPitchMin_deg    = -180.f;
  Point3f point;
  bool inlier;
  Point2f angles;
  FaceDirectionData3d()
    :
      point(Point3f(kDefaultDistance_cm, kDefaultDistance_cm, kDefaultDistance_cm)),
      inlier(false),
      angles(Point2f(kYawMin_deg, kPitchMin_deg))
      {}
  void Update(const Point3f& p, float pitch, float yaw)
  {
    point = p;
    inlier = false;
    angles = Point2f(yaw, pitch); 
  }
};

class FaceNormalDirectedAtRobot3d
{
public:
  FaceNormalDirectedAtRobot3d();


  void Update(const TrackedFace& headPose,
              const TimeStamp_t timeStamp);

  TrackedFace::FaceDirection GetFaceDirection() const {return _faceDirection;}
  Point3f GetFaceDirectionAverage() const {return _faceDirectionAverage;}
  bool GetExpired(const TimeStamp_t currentTime) const;
  std::vector<FaceDirectionData3d> const& GetFaceDirectionHistory() {return _faceDirectionHistory;}

private:
  int FindInliers(const Point3f& faceDirectionAverage);

  TrackedFace::FaceDirection DetermineFaceDirection();

  Point3f GetPointFromHeadPose(const Pose3d& headPose);
  Point3f ComputeEntireFaceDirectionAverage();
  Point3f ComputeFaceDirectionAverage(const bool filterOutliers);
  Point3f RecomputeFaceDirectionAverage();
  Point3f IntersectionDirectionWithGroundPlane(const Point3f& a, const Point3f b, const float distance);

  bool IsFaceDirectionAboveHorizon();

  Pose3d _headPose;
  TimeStamp_t _lastUpdated;

  int _currentIndex = 0;
  int _numberOfInliers = 0;
  TrackedFace::FaceDirection _faceDirection;
  bool _initialized = false;

  std::vector<FaceDirectionData3d> _faceDirectionHistory;
  Point3f _faceDirectionAverage;
};

}
}
#endif // __Anki_Vision_FaceNormalDirectedAtRobot_H__
