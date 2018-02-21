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

#include <cmath>

namespace Anki {

namespace Vision {

  struct GazeData {
    Point2f point;
    bool inlier;
    GazeData()
      : point(Point2f(-30.f, -20.f)),
        inlier(false) {}
    void Update(Point2f newPoint) {
      point = newPoint;
      inlier = false;
    }
  };

  class EyeContact {

  public:
    EyeContact();

    void Update(const TrackedFace& face,
                const TimeStamp_t& timeStamp);

    bool GetEyeContact() {return _eyeContact;}
    Point2f GetGazeAverage() {return _gazeAverage;}
    bool GetFixating();
    bool GetExpired(const TimeStamp_t& currentTime);
    std::vector<GazeData> GetHistory() {return _gazeHistory;}

  private:
    int FindInliers(const Point2f gazeAverage);

    bool MakingEyeContact();

    Point2f GazeAverage();

    Point2f RecomputeGazeAverage();

    bool SecondaryContraints();

    TrackedFace _face;
    TimeStamp_t _lastUpdated;

    int _currentIndex = 0;
    int _numberOfInliers = 0;
    bool _eyeContact = false;
    bool _initialized = false;

    std::vector<GazeData> _gazeHistory;
    Point2f _gazeAverage;
  };

} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_EyeContact_H__
