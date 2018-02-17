/**
 * File: eyeContact.cpp
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
#include "eyeContact.h"

#include "coretech/common/shared/types.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vision {

  namespace {
    // TODO where do we get the short names for the types
    CONSOLE_VAR(u32, kHistorySize,                    "Vision.EyeContact",  6);
    CONSOLE_VAR(f32, kInlierDistance,                 "Vision.EyeContact",  10.f);
    CONSOLE_VAR(u32, kMinNumberOfInliers,             "Vision.EyeContact",  3);
    CONSOLE_VAR(f32, kEyeContactDistance,             "Vision.EyeContact",  8.f);
    // TODO the threshold here is pi/2 really should just use a better
    // approximation of pi
    CONSOLE_VAR(f32, kPitchAngleThreshold,            "Vision.EyeContact",  M_PI_F/2.f);
    CONSOLE_VAR(f32, kYawAngleThreshold,              "Vision.EyeContact",  M_PI_F/2.f);
    // TODO these might need to be adaptive depening on data
    CONSOLE_VAR(f32, kBlinkAmountThreshold,           "Vision.EyeContact",  73.f);
    CONSOLE_VAR(f32, kDistanceFromCameraThresholdmm,  "Vision.EyeContact",  700.f);
    CONSOLE_VAR(u32, kExpireThreshold,                "Vision.EyeContact",  50);
  }

  void EyeContact::Update(const TrackedFace& face, 
        const TimeStamp_t& timeStamp) {
    _face = face;
    _lastUpdated = timeStamp;

    if (_currentIndex > (kHistorySize - 1)) {
      // Make sure we place a "real" value
      // in every element of the history
      // before returning a result
      _initialized = true;
      _currentIndex = 0;
    }

    // First add our current point to the history
    _gazeHistory[_currentIndex].Update(
      Point2f(
        _face.GetGaze().leftRight_deg,
        _face.GetGaze().upDown_deg
      )
    );

    _gazeAverage = GazeAverage();
    _numberOfInliers = FindInliers(_gazeAverage);
    _gazeAverage = RecomputeGazeAverage();
    _eyeContact = MakingEyeContact();

    _currentIndex += 1;
  }

  Point2f EyeContact::GazeAverage() {
    float averageGazeX = 0.f;
    float averageGazeY = 0.f;
    for (const auto& gaze: _gazeHistory) {
      averageGazeX += gaze.point[0];
      averageGazeY += gaze.point[1];
    }
    averageGazeX /= _gazeHistory.size();
    averageGazeY /= _gazeHistory.size();
    return Point2f(averageGazeX, averageGazeY);
  }

  int EyeContact::FindInliers(const Point2f gazeAverage) {
    int numberOfInliers = 0;
    int index = 0;
    for (auto& gaze: _gazeHistory) {
      auto differenceX = gaze.point[0] - gazeAverage[0];
      auto differenceY = gaze.point[1] - gazeAverage[1];
      auto distance = std::sqrt(differenceX*differenceX
        + differenceY*differenceY);
      if (distance < kInlierDistance) {
        gaze.inlier = true;
        numberOfInliers += 1;
      } else {
        gaze.inlier = false;
      }
      index += 1;
    }
    return numberOfInliers;
  }

  Point2f EyeContact::RecomputeGazeAverage() {
    float averageGazeX = 0.f;
    float averageGazeY = 0.f;
    int index = 0;
    for (const auto& gaze: _gazeHistory) {
      if (gaze.inlier) {
        averageGazeX += gaze.point[0];
        averageGazeY += gaze.point[1];
      }
      index += 1;
    }
    averageGazeX /= _numberOfInliers;
    averageGazeY /= _numberOfInliers;
    return Point2f(averageGazeX, averageGazeY);
  }

  bool EyeContact::MakingEyeContact() {
    bool eyeContact = false;
    if (_numberOfInliers > kMinNumberOfInliers) {
      // Taking the l_2 norm of the gaze average here works
      // because the distance is from (0,0)
      float distance = std::sqrt(_gazeAverage[0]*_gazeAverage[0] +
        _gazeAverage[1]*_gazeAverage[1]);
      if ((distance < kEyeContactDistance) && _initialized && SecondaryContraints()) {
          eyeContact = true;
      }
    }
    return eyeContact;
  }

  bool EyeContact::SecondaryContraints() {
    // Verify that the head in the required cone
    bool headRoationInCone = (std::abs(_face.GetHeadPitch().ToFloat()) < kPitchAngleThreshold)
        && (std::abs(_face.GetHeadYaw().ToFloat()) < kYawAngleThreshold);

    // Verify that the face ins't blinking
    // TODO we might want to consider using an adpative threshold here
    // depending on how well the blink amount varifies per
    // individual
    bool blinking = (_face.GetBlinkAmount().blinkAmountLeft > kBlinkAmountThreshold) || 
        (_face.GetBlinkAmount().blinkAmountRight > kBlinkAmountThreshold);

    bool near = _face.GetHeadPose().GetTranslation().Length() < kDistanceFromCameraThresholdmm;

    return headRoationInCone && blinking && near;
  }

  bool EyeContact::GetFixating() {
    return _numberOfInliers < kMinNumberOfInliers;
  }

  bool EyeContact::GetExpired(const TimeStamp_t& currentTime) {
    if ((currentTime - _lastUpdated) > kExpireThreshold) {
      return true;
    } else {
      return false;
    }
  }
} // namespace Vision
} // namespace Anki
