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

#include "anki/cozmo/basestation/actionInterface.h"
#include "anki/vision/basestation/trackedFace.h"

#include "clad/types/actionTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward Declarations:
class Robot;
class DriveStraightAction;

class ITrackAction : public IAction
{
public:
  
  enum class Mode {
    HeadAndBody,
    HeadOnly,
    BodyOnly
  };
  
  // Choose whether to track with head, body, or both (default)
  void SetMode(Mode newMode) { _mode = newMode; }
  Mode GetMode() const { return _mode; }
  
  // If pan and tilt angles for tracking are *both* below this threshold,
  // the robot will drive straight the specified distance at the specified speed.
  // Set to zero to disable (default). This is independent of tracking Mode.
  void SetDriveForward(const Radians& threshold, f32 distance_mm,
                       f32 speed_mmps = 1.5f*DEFAULT_PATH_SPEED_MMPS,
                       f32 accel_mmps2 = 1.5f*DEFAULT_PATH_ACCEL_MMPS2);
  
  const Radians& GetDriveForwardThreshold() const { return _driveForwardThreshold; }
  f32 GetDriveForwardDistance() const { return _driveForwardDistance; }
  
  // Set an animation to play upon first detection of whatever is being tracked.
  void SetInitialReactionAnimation(const std::string& name) { _initialReactionAnimation = name; }
  
  virtual u8 GetAnimTracksToDisable() const override;
  
protected:

  // Derived classes must implement Init(), but cannot implement CheckIfDone().
  virtual ActionResult CheckIfDone(Robot& robot) override final;
  
  // Implementation-specific method for computing the vector from the robot
  // to whatever is being tracked. The pan/tilt angles will be set from this vector.
  // Return true if new vector provided, false if same as last time
  virtual bool GetTrackingVector(Robot& robot, Vec3f& newTrackingVector) = 0;
  
  virtual void Cleanup(Robot& robot) override;
  
private:
  
  Mode     _mode = Mode::HeadAndBody;
  Radians  _driveForwardThreshold = 0.f;
  f32      _driveForwardDistance = 20.f;
  f32      _driveForwardSpeed = DEFAULT_PATH_SPEED_MMPS * 1.5f;
  f32      _driveForwardAccel = DEFAULT_PATH_ACCEL_MMPS2 * 1.5f;
  
  bool        _initialReactionAnimPlayed = false;
  std::string _initialReactionAnimation;
  
  DriveStraightAction* _driveForwardAction = nullptr;
  
}; // class ITrackAction
  
inline void ITrackAction::SetDriveForward(const Radians& threshold, f32 distance_mm,
                                          f32 speed_mmps, f32 accel_mmps2)
{
  _driveForwardThreshold = threshold;
  _driveForwardDistance = distance_mm;
  _driveForwardSpeed = speed_mmps;
  _driveForwardAccel = accel_mmps2;
}


class TrackObjectAction : public ITrackAction
{
public:
  TrackObjectAction(const ObjectID& objectID, bool trackByType = true);
  
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return RobotActionType::TRACK_OBJECT; }

protected:
  
  virtual ActionResult Init(Robot& robot) override;
  
  // Required by ITrackAction:
  virtual bool GetTrackingVector(Robot& robot, Vec3f& newTrackingVector) override;
  
private:
  
  ObjectID             _objectID;
  ObjectType           _objectType;
  bool                 _trackByType;
  std::string          _name = "TrackObjectAction";
  Pose3d               _lastTrackToPose;
  
}; // class TrackObjectAction

  
class TrackFaceAction : public ITrackAction
{
public:
  
  using FaceID = Vision::TrackedFace::ID_t;
  
  TrackFaceAction(FaceID faceID);
  
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return RobotActionType::TRACK_FACE; }

protected:
  
  virtual ActionResult Init(Robot& robot) override;
  
  // Required by ITrackAction:
  virtual bool GetTrackingVector(Robot& robot, Vec3f& newTrackingVector) override;
  
private:

  FaceID               _faceID;
  std::string          _name = "TrackFaceAction";


}; // class TrackFaceAction

  
class TrackMotionAction : public ITrackAction
{
public:
  
  TrackMotionAction() { }
  
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return RobotActionType::TRACK_MOTION; }
  
protected:
  
  virtual ActionResult Init(Robot& robot) override;
  
  // Required by ITrackAction:
  virtual bool GetTrackingVector(Robot& robot, Vec3f& newTrackingVector) override;
  
private:
  
  std::string _name = "TrackMotionAction";
  
  // TODO: Provide setters for these
  f32     _holdHeadDownUntil = -1.f;
  f32     _holdHeadDownDuration = 1.5f;
  f32     _minDriveFrowardGroundPlaneDist_mm = 93.1f;
  f32     _minGroundAreaToConsider = 0.1f;
  
  Radians _origDriveForwardThreshold;
  f32     _origDriveForwardDistance;
  bool _gotNewMotionObservation = false;
  
  ExternalInterface::RobotObservedMotion _motionObservation;
  
  Signal::SmartHandle _signalHandle;
  
  std::string _previousIdleAnimation;
  
}; // class TrackMotionAction
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_TrackingActions_H__
