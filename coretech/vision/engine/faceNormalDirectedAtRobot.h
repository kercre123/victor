/**
 * File: faceNormalDirectedAtRobot.h
 *
 * Author: Robert Cosgriff
 * Date:   9/5/2018
 *
 * Description: Class that determines if a face normal is directed at the robot.
 *              In plain language, whether a person's head is "facing" the robot.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Vision_FaceNormalDirectedAtRobot_H__
#define __Anki_Vision_FaceNormalDirectedAtRobot_H__

#include "coretech/vision/engine/trackedFace.h"

namespace Anki {
namespace Vision {

struct FaceDirectionData
{
  constexpr static const float kYawMin_deg      = -180.f;
  constexpr static const float kPitchMin_deg    = -180.f;
  Point2f point;
  bool inlier;
  FaceDirectionData()
    : point(Point2f(kYawMin_deg, kPitchMin_deg)),
      inlier(false) {}
  void Update(const Point2f& newPoint)
  {
    point = newPoint;
    inlier = false;
  }
};



class FaceNormalDirectedAtRobot
{
public:
  FaceNormalDirectedAtRobot();


  void Update(const TrackedFace& face,
              const TimeStamp_t timeStamp);

  TrackedFace::FaceDirection GetFaceDirection() const {return _faceDirection;}
  Point2f GetFaceDirectionAverage() const {return _faceDirectionAverage;}
  bool GetExpired(const TimeStamp_t currentTime) const;
  std::vector<FaceDirectionData> const& GetFaceDirectionHistory() {return _faceDirectionHistory;}

private:
  int FindInliers(const Point2f& faceDirectionAverage);

  TrackedFace::FaceDirection DetermineFaceDirection();

  Point2f ComputeEntireFaceDirectionAverage();

  Point2f ComputeFaceDirectionAverage(const bool filterOutliers);

  Point2f RecomputeFaceDirectionAverage();

  TrackedFace _face;
  TimeStamp_t _lastUpdated;

  int _currentIndex = 0;
  int _numberOfInliers = 0;
  TrackedFace::FaceDirection _faceDirection;
  bool _initialized = false;

  std::vector<FaceDirectionData> _faceDirectionHistory;
  Point2f _faceDirectionAverage;
};

}
}
#endif // __Anki_Vision_FaceNormalDirectedAtRobot_H__
