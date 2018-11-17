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

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR(u32, kHistorySize,                      "Vision.EyeContact",  6);
  CONSOLE_VAR(f32, kInlierDistanceSq,                 "Vision.EyeContact",  100.f);
  CONSOLE_VAR(u32, kMinNumberOfInliers,               "Vision.EyeContact",  3);
  CONSOLE_VAR(f32, kEyeContactDistanceSq,             "Vision.EyeContact",  64.f);
  // TODO the threshold here is pi/2 really should just use a better
  // approximation of pi
  CONSOLE_VAR(f32, kPitchAngleThreshold_rad,          "Vision.EyeContact",  M_PI_F/2.f);
  CONSOLE_VAR(f32, kYawAngleThreshold_rad,            "Vision.EyeContact",  M_PI_F/2.f);
  // TODO these might need to be adaptive depending on data
  CONSOLE_VAR(f32, kBlinkAmountThreshold,             "Vision.EyeContact",  .73f);
  CONSOLE_VAR(f32, kDistanceFromCameraThresholdSq_mm, "Vision.EyeContact",  700*700.f);
  CONSOLE_VAR(u32, kExpireThreshold,                  "Vision.EyeContact",  50);
}

EyeContact::EyeContact()
  : _gazeAverage(Point2f(GazeData::kLeftRightMin_deg, GazeData::kUpDownMin_deg))
{
  _gazeHistory.resize(kHistorySize);
}

void EyeContact::Update(const TrackedFace& face,
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
  _gazeHistory[_currentIndex].Update(
    Point2f(
      _face.GetGaze().leftRight_deg,
      _face.GetGaze().upDown_deg
    )
  );

  _gazeAverage = ComputeEntireGazeAverage();
  _numberOfInliers = FindInliers(_gazeAverage);
  _gazeAverage = RecomputeGazeAverage();
  _isMakingEyeContact = DetermineMakingEyeContact();

  _currentIndex += 1;
}

Point2f EyeContact::ComputeEntireGazeAverage()
{
  return ComputeGazeAverage(false);
}

Point2f EyeContact::ComputeGazeAverage(const bool filterOutliers)
{
  Point2f averageGaze = Point2f(0.f, 0.f);

  // Find the average, we can treat these as Cartesian
  // coordinates instead traditional angles because the
  // range of value does not include a wrap around
  u32 pointsInAverage = 0;
  for (const auto& gaze: _gazeHistory) {
    if (filterOutliers) {
      if (gaze.inlier) {
        pointsInAverage += 1;
        averageGaze += gaze.point;
      }
    } else {
      pointsInAverage += 1;
      averageGaze += gaze.point;
    }
  }

  if (pointsInAverage != 0) {
    averageGaze *= 1.f/pointsInAverage;
  } else {
    averageGaze = Point2f(GazeData::kLeftRightMin_deg, GazeData::kUpDownMin_deg);
  }
  return averageGaze;
}

int EyeContact::FindInliers(const Point2f& gazeAverage)
{
  int numberOfInliers = 0;
  for (auto& gaze: _gazeHistory) {
    auto difference = gaze.point - gazeAverage;
    auto distance = difference.LengthSq();
    if (distance < kInlierDistanceSq) {
      gaze.inlier = true;
      numberOfInliers += 1;
    } else {
      gaze.inlier = false;
    }
  }
  return numberOfInliers;
}

Point2f EyeContact::RecomputeGazeAverage()
{
  return ComputeGazeAverage(true);
}

bool EyeContact::DetermineMakingEyeContact()
{
  bool eyeContact = false;
  if (_numberOfInliers > kMinNumberOfInliers) {
    // Taking the l_2 norm of the gaze average here works
    // because the distance is from (0,0)
    float distance = _gazeAverage.LengthSq();
    if ((distance < kEyeContactDistanceSq) && _initialized && SecondaryContraints()) {
      eyeContact = true;
    }
  }
  return eyeContact;
}

bool EyeContact::SecondaryContraints()
{
  // Verify that the head in the required cone. We don't need to worry
  // about wrap around here. The okao library ranges for these values
  // are from [-180, 179] degrees.
  const bool headRotationInCone = ((std::abs(_face.GetHeadPitch().ToFloat()) < kPitchAngleThreshold_rad) &&
                                   (std::abs(_face.GetHeadYaw().ToFloat()) < kYawAngleThreshold_rad));

  // Verify that the face isn't blinking
  // TODO we might want to consider using an adaptive threshold here
  // depending on how much the blink amount varies per individual
  const bool blinking = ((_face.GetBlinkAmount().blinkAmountLeft < kBlinkAmountThreshold) ||
                         (_face.GetBlinkAmount().blinkAmountRight < kBlinkAmountThreshold));

  const bool near = (_face.GetHeadPose().GetTranslation().LengthSq() < kDistanceFromCameraThresholdSq_mm);

  return (headRotationInCone && blinking && near);
}

bool EyeContact::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kExpireThreshold;
}
} // namespace Vision
} // namespace Anki
