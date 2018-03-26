/**
 * File: eyeContact.h
 *
 * Author: Robert Cosgriff
 * Date:   2/2/2018
 *
 * Description: Class that determines whether a face that
 *              we have found is making eye contact with
 *              the camera.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Vision_EyeContact_H__
#define __Anki_Vision_EyeContact_H__

#include "coretech/vision/engine/trackedFace.h"

namespace Anki {
namespace Vision {

// These default values are at the limits of the expected range
// of the inputs. Since we are looking for when the average
// is close to zero, the extremes of the range were chosen for
// initialization values.

struct GazeData
{
  constexpr static const float kleftRightMin_deg = -30.f;
  constexpr static const float kupDownMin_deg    = -20.f;
  Point2f point;
  bool inlier;
  GazeData()
    : point(Point2f(kleftRightMin_deg, kupDownMin_deg)),
      inlier(false) {}
  void Update(const Point2f& newPoint)
  {
    point = newPoint;
    inlier = false;
  }
};

/***************************************************
 *                 EyeContact                      *
 ***************************************************/
/*
  This class' primary input is gaze estimation from the okao library. These
  inputs are the angles the gaze is making with the image plane. There
  are two angles: the horizontal angle (left right), and vertical (up down).
  The range of the these inputs are +/-30 degrees for the horizontal angle,
  and +/-20 degrees for the vertical angle. Both of these ranges are from
  the okoa documentation; however in practice angles occasionally
  exceed this range. Due to this limited range we don't need to address
  typical issues when comparing angles, and for this class these inputs
  are treated as Cartesian coordinates.

  The goal of this class is to find whether both these inputs are
  sufficiently close to a value of 0. To achieve this a simple running
  average with inliers, followed by a threshold check for proximity to
  the origin is used.

  Specifically:
    - The average gaze is computed for the entire history
    - The number of inliers is found using a given threshold
    - The gaze average is then "refined" by computing the average
      only over the inliers
    - Check if there is a sufficient number of inliers, whether
      the refined average is close enough to the origin, and
      whether the secondary constraints are satisfied.
      - Secondary Constraints:
        - The head is pitch, and yaw are within a given range
        - The blink amount is less than a given threshold (a large
          blink amount means the eye is close to being fully closed).
        - The face is sufficiently close to the camera per the
          translation of the face determined from the distance between
          eyes

  Additionally, there is a notion of a expiration of the history. However,
  this is only a condition and nothing is done internally to handle it.

  As far as usage, the value returned from GetGazeAverage is only valid
  if IsMakingEyeContact returns true.
*/

class EyeContact
{
public:
  EyeContact();

  void Update(const TrackedFace& face,
              const TimeStamp_t timeStamp);

  bool IsMakingEyeContact() const {return _isMakingEyeContact;}
  Point2f GetGazeAverage() const {return _gazeAverage;}
  bool GetExpired(const TimeStamp_t currentTime) const;
  std::vector<GazeData> const& GetGazeHistory() {return _gazeHistory;}

private:
  int FindInliers(const Point2f& gazeAverage);

  bool DetermineMakingEyeContact();

  Point2f ComputeEntireGazeAverage();

  Point2f ComputeGazeAverage(const bool filterOutliers);

  Point2f RecomputeGazeAverage();

  bool SecondaryContraints();

  TrackedFace _face;
  TimeStamp_t _lastUpdated;

  int _currentIndex = 0;
  int _numberOfInliers = 0;
  bool _isMakingEyeContact = false;
  bool _initialized = false;

  std::vector<GazeData> _gazeHistory;
  Point2f _gazeAverage;
};

} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_EyeContact_H__
