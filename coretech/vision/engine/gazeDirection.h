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

struct GazeDirectionData
{
  constexpr static const float kDefaultDistance_cm = -100000.f;
  constexpr static const float kYawMin_deg      = -180.f;
  constexpr static const float kPitchMin_deg    = -180.f;
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

  Point3f GetFaceDirectionSurfaceAverage() const;
  Point3f GetCurrentFaceDirectionSurface() const;
  std::vector<GazeDirectionData> const& GetFaceDirectionHistory() {return _faceDirectionSurfaceHistory;}

  Point3f GetEyeDirectionAverage() const;
  Point3f GetCurrentEyeDirection() const;
  std::vector<GazeDirectionData> const& GetEyeDirectionHistory() {return _eyeDirectionHistory;}

  Point3f GetTotalGazeAvergae() const;

  bool GetExpired(const TimeStamp_t currentTime) const;
  bool IsFaceFocused() const;

  void ClearHistory();

private:
  int FindInliers(const Point3f& faceDirectionAverage);
  int FindEyeDirectionInliers(const Point3f& eyeDirectionAverage);

  bool GetPointFromHeadPose(const Pose3d& headPose, Point3f& faceDirectionPoint);
  Point3f ComputeEntireFaceDirectionAverage();
  Point3f ComputeFaceDirectionAverage(const bool filterOutliers);
  Point3f RecomputeFaceDirectionAverage();

  bool GetPointFromEyePose(const Pose3d& eyePose, Point3f& eyeDirectionPoint);
  Point3f ComputeEntireEyeDirectionAverage();
  Point3f ComputeEyeDirectionAverage(const bool filterOutliers);
  Point3f RecomputeEyeDirectionAverage();

  void DoEyeAndHeadAgree(const Point3f& headPointAverage, const Point3f& eyePointAverage);

  Pose3d _headPose;
  Pose3d _eyePose;
  TimeStamp_t _lastUpdated;

  int _currentIndex = 0;
  int _numberOfInliers = 0;
  int _numberOfEyeDirectionsInliers = 0;
  bool _initialized = false;

  std::vector<GazeDirectionData> _faceDirectionSurfaceHistory;
  Point3f _faceDirectionSurfaceAverage;

  std::vector<GazeDirectionData> _eyeDirectionHistory;
  Point3f _eyeDirectionAverage;

  bool _gazeAgreement;
};

}
}
#endif // __Anki_Vision_FaceNormalDirectedAtRobot_H__
