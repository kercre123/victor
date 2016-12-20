/**
 * File: trackingActions.h
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an interface and specific actions for tracking, derived 
 *              from the general IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Cozmo_Basestation_TrackingActions_H__
#define __Anki_Cozmo_Basestation_TrackingActions_H__

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/vision/basestation/trackedFace.h"

#include "clad/types/actionTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward Declarations:
class Robot;
class RobotPoseStamp;
class DriveStraightAction;

template <typename Type>
class AnkiEvent;

class ITrackAction : public IAction
{
public:
  
  enum class Mode {
    HeadAndBody,
    HeadOnly,
    BodyOnly
  };
  
  // Choose whether to track with head, body, or both (default)
  void SetMode(Mode newMode);
  Mode GetMode() const { return _mode; }
  
  // Set how long the tracker will run without seeing whatever it is trying to
  // track. After this, it will complete "successfully".
  // Set to 0 to disable timeout (default).
  void SetUpdateTimeout(double timeout_sec) { _updateTimeout_sec = timeout_sec; }

  // Tells this action to keep running until another action (being run separately) stops. As soon as this
  // other action completes, this action will complete as well
  void StopTrackingWhenOtherActionCompleted( u32 otherActionTag );
  
  // Set min/max speeds
  void SetTiltSpeeds(f32 minSpeed_radPerSec, f32 maxSpeed_radPerSec);
  void SetPanSpeeds(f32 minSpeed_radPerSec,  f32 maxSpeed_radPerSec);

  // Sound settings: which animation (should be sound only), how frequent, and
  // minimum angle required to play sound. Use AnimationTrigger::Count for sound to
  // disable. (Note that there *is* sound by default.)
  void SetSound(const AnimationTrigger animName) { _turningSoundAnimTrigger = animName; }
  void SetSoundSpacing(f32 spacingMin_sec, f32 spacingMax_sec);
  void SetMinPanAngleForSound(const Radians& angle) { _minPanAngleForSound = angle.getAbsoluteVal(); }
  void SetMinTiltAngleForSound(const Radians& angle) { _minTiltAngleForSound = angle.getAbsoluteVal(); }
  
  // Tracking is meant to be ongoing, so "never" timeout
  virtual f32 GetTimeoutInSeconds() const override { return std::numeric_limits<f32>::max(); }
  
  // Angles returned by GetAngles() method must be greater than these tolerances
  // to actually trigger movement.
  void SetPanTolerance(const Radians& panThreshold);
  void SetTiltTolerance(const Radians& tiltThreshold);

  // If enabled, angles returned by GetAngles() below tolerance (which would generally
  // be ignored) are clamped to the tolerances so that the robot _always_ moves
  // by at least the tolerance amounts (in the correct direction). This creates
  // extra (technically, unncessary) movement of the robot, but keeps him looking
  // more alive while tracking. Disabled by default.
  void SetClampSmallAnglesToTolerances(bool tf) { _clampSmallAngles = tf; }
  
  void SetMaxHeadAngle(const Radians& maxHeadAngle_rads) { _maxHeadAngle = maxHeadAngle_rads; }

  // Enable/disable moving of eyes while tracking. Default is false.
  void SetMoveEyes(bool moveEyes) { _moveEyes = moveEyes; }
  
protected:

  ITrackAction(Robot& robot, const std::string name, const RobotActionType type);
  virtual ~ITrackAction();
  
  // Note that derived classes should override InitInternal, which is called by Init
  virtual ActionResult Init() override final;
  virtual ActionResult InitInternal() = 0;
  
  // Derived classes must implement Init(), but cannot implement CheckIfDone().
  virtual ActionResult CheckIfDone() override final;
  
  // Implementation-specific method for computing the absolute angles needed
  // to turn and face whatever is being tracked.
  // Return true if new angles were provided, false if same as last time.
  virtual bool GetAngles(Radians& absPanAngle, Radians& absTiltAngle) = 0;
  
  virtual bool InterruptInternal() override final;
  
private:
  
  Mode     _mode = Mode::HeadAndBody;
  double   _updateTimeout_sec = 0.;
  double   _lastUpdateTime = 0.;
  Radians  _panTolerance  = POINT_TURN_ANGLE_TOL;
  Radians  _tiltTolerance = HEAD_ANGLE_TOL;
  Radians  _maxHeadAngle  = MAX_HEAD_ANGLE;
  u32      _stopOnOtherActionTag = ActionConstants::INVALID_TAG;
  
  u32      _eyeShiftTag;
  bool     _moveEyes    = false;
  f32      _originalEyeDartDist;
  AnimationTrigger _turningSoundAnimTrigger = AnimationTrigger::SoundOnlyTurnSmall;
  f32      _soundSpacingMin_sec = 0.5f;
  f32      _soundSpacingMax_sec = 1.0f;
  f32      _nextSoundTime = 0.f;
  Radians  _minPanAngleForSound = DEG_TO_RAD(10);
  Radians  _minTiltAngleForSound = DEG_TO_RAD(10);
  
  f32      _minTiltSpeed_radPerSec = DEG_TO_RAD(30.f);
  f32      _maxTiltSpeed_radPerSec = DEG_TO_RAD(90.f);
  f32      _minPanSpeed_radPerSec  = DEG_TO_RAD(40.f);
  f32      _maxPanSpeed_radPerSec  = DEG_TO_RAD(120.f);
  
  u32      _soundAnimTag = (u32)ActionConstants::INVALID_TAG;
  bool     _clampSmallAngles = false;
  
}; // class ITrackAction
  
inline void ITrackAction::SetSoundSpacing(f32 spacingMin_sec, f32 spacingMax_sec) {
  _soundSpacingMin_sec = spacingMin_sec;
  _soundSpacingMax_sec = spacingMax_sec;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TrackObjectAction : public ITrackAction
{
public:
  TrackObjectAction(Robot& robot, const ObjectID& objectID, bool trackByType = true);
  virtual ~TrackObjectAction();

protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual bool GetAngles(Radians& absPanAngle, Radians& absTiltAngle) override;
  
private:
  
  ObjectID             _objectID;
  ObjectType           _objectType;
  bool                 _trackByType;
  Pose3d               _lastTrackToPose;
  
}; // class TrackObjectAction

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TrackFaceAction : public ITrackAction
{
public:
  
  using FaceID = Vision::FaceID_t;
  
  TrackFaceAction(Robot& robot, FaceID faceID);
  virtual ~TrackFaceAction();

  virtual void GetCompletionUnion(ActionCompletedUnion& completionInfo) const override;
  
protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual bool GetAngles(Radians& absPanAngle, Radians& absTiltAngle) override;
  
private:

  FaceID               _faceID;
  TimeStamp_t          _lastFaceUpdate = 0;

  Signal::SmartHandle _signalHandle;

}; // class TrackFaceAction
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TrackPetFaceAction : public ITrackAction
{
public:
  
  using FaceID = Vision::FaceID_t;
  
  // Track a specific pet ID
  TrackPetFaceAction(Robot& robot, FaceID faceID);
  
  // Track first pet with the right type (or any pet at all if PetType set to Unknown).
  // Note the pet being tracked could change during tracking as it is the first one
  // found in PetWorld on each update tick.
  TrackPetFaceAction(Robot& robot, Vision::PetType petType);
  
  virtual void GetCompletionUnion(ActionCompletedUnion& completionInfo) const override;
  
protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual bool GetAngles(Radians& absPanAngle, Radians& absTiltAngle) override;
  
private:
  
  FaceID               _faceID  = Vision::UnknownFaceID;
  Vision::PetType      _petType = Vision::PetType::Unknown;
  TimeStamp_t          _lastFaceUpdate = 0;
  
}; // class TrackPetFaceAction

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TrackMotionAction : public ITrackAction
{
public:
  
  TrackMotionAction(Robot& robot) : ITrackAction(robot, "TrackMotion", RobotActionType::TRACK_MOTION) { }
  
protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual bool GetAngles(Radians& absPanAngle, Radians& absTiltAngle) override;
  
private:
  
  bool _gotNewMotionObservation = false;
  
  ExternalInterface::RobotObservedMotion _motionObservation;
  
  Signal::SmartHandle _signalHandle;
  
}; // class TrackMotionAction
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_TrackingActions_H__
