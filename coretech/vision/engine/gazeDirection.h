/**
 * File: gazeDirection.h
 *
 * Author: Robert Cosgriff
 * Date:   11/26/2018
 *
 * Description: Class that determines where on the ground plane a persons gaze
 *              is directed. This only uses the head rotation computed from
 *              face parts at the moment, and no information from the eyes.
 *              Thus this is not a true estimation of gaze merely a rough approximation.
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_CoreTech_Vision_GazeDirection_H__
#define __Anki_CoreTech_Vision_GazeDirection_H__

#include "coretech/vision/engine/trackedFace.h"

namespace Anki {
namespace Vision {

struct GazeDirectionData
{
  constexpr static const float kDefaultDistance_mm = -100000.f;
  Point3f point;
  bool inlier;
  bool include;

  GazeDirectionData()
    : point(kDefaultDistance_mm, kDefaultDistance_mm, kDefaultDistance_mm)
      ,inlier(false)
      ,include(false)
      {
      }

  void Update(const Point3f& p, bool i)
  {
    point = p;
    inlier = false;
    include = i;
  }
};

/***************************************************
 *                 GazeDirection                   *
 ***************************************************/
/*
  This class' primary input is head rotation estimation from the okao library.
  This head rotation which can also be though of as a face "normal" is then
  projected onto the ground plane (if such a projection exists) into a point
  in world coordinates. These points are stored in a buffer.

  The goal of this class is to find whether these gaze ground plane points
  are sufficiently "stable" or stationary such that it is clear that the head
  is "gazing" at a point on the ground plane. To achieve this a simple running
  average with inliers is used, followed by a threshold check for proximity to
  the average.

  Specifically:
    - The average gaze point is computed for the entire history
    - The number of inliers is found using a given threshold
    - The gaze point average is then "refined" by computing the average
      only over the inliers
    - Check if there is a sufficient number of inliers, and if so
      the gaze point is considered "stable".

  Additionally, there is a notion of a expiration of the history. However,
  this is only a condition and nothing is done internally to handle it.

  As far as usage, the value returned from GetGazeDirectionAverage is only valid
  if IsStable returns true. ClearHistory is method that is intended to be called
  when the robot moves, or if a new gaze point estimate should start immeadiately
  instead of waiting for the points in the history to expire.
*/

class GazeDirection
{
public:
  GazeDirection();

  void Update(const TrackedFace& headPose);

  // This will return the average found amoung the inliers
  // this is only accurate if IsStable returns true.
  Point3f GetGazeDirectionAverage() const;
  // This will return the last point in the history and
  // is used for debugging and visualization purposes.
  Point3f GetCurrentGazeDirection() const;

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

  TimeStamp_t _lastUpdated = 0;
  int _currentIndex = 0;
  int _numberOfInliers = 0;
  bool _initialized = false;

  std::vector<GazeDirectionData> _gazeDirectionHistory;
  Point3f _gazeDirectionAverage;
};

}
}
#endif // __Anki_CoreTech_Vision_GazeDirection_H__
