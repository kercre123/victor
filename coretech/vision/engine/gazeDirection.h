/**
 * File: gazeDirection.h
 *
 * Author: Robert Cosgriff
 * Date:   11/26/2018
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

struct GazeDirectionData
{
  constexpr static const float kDefaultDistance_cm = -100000.f;
  constexpr static const float kYawMin_deg         = -180.f;
  constexpr static const float kPitchMin_deg       = -180.f;
  Point3f point;
  bool inlier;
  bool include;
  Point2f angles;
  GazeDirectionData()
    :
      point(Point3f(kDefaultDistance_cm, kDefaultDistance_cm, kDefaultDistance_cm)),
      inlier(false),
      include(false),
      angles(Point2f(kYawMin_deg, kPitchMin_deg))
      {}
  // TODO naming is terrible
  void Update(const Point3f& p, float yaw, float pitch, bool i)
  {
    point = p;
    inlier = false;
    angles = Point2f(yaw, pitch);
    include = i;
  }
};

class GazeDirection
{
public:
  GazeDirection();


  void Update(const TrackedFace& headPose,
              const TimeStamp_t timeStamp);

  Point3f GetGazeDirectionAverage() const;
  Point3f GetCurrentGazeDirection() const;
  std::vector<GazeDirectionData> const& GetFaceDirectionHistory() {return _gazeDirectionHistory;}

  bool GetExpired(const TimeStamp_t currentTime) const;
  bool IsStable() const;

  void ClearHistory();

private:
  int FindInliers(const Point3f& faceDirectionAverage);

  bool GetPointFromHeadPose(const Pose3d& headPose, Point3f& faceDirectionPoint);
  Point3f ComputeEntireGazeDirectionAverage();
  Point3f ComputeGazeDirectionAverage(const bool filterOutliers);
  Point3f RecomputeGazeDirectionAverage();

  Pose3d _headPose;
  TimeStamp_t _lastUpdated;

  int _currentIndex = 0;
  int _numberOfInliers = 0;
  bool _initialized = false;

  std::vector<GazeDirectionData> _gazeDirectionHistory;
  Point3f _gazeDirectionAverage;
};

}
}
#endif // __Anki_Vision_FaceNormalDirectedAtRobot_H__
