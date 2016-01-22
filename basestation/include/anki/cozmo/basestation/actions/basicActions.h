/**
 * File: basicActions.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements basic cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASIC_ACTIONS_H
#define ANKI_COZMO_BASIC_ACTIONS_H

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/types/actionTypes.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  
  namespace Cozmo {

    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public IAction
    {
    public:
      TurnInPlaceAction(const Radians& angle, const bool isAbsolute);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::TURN_IN_PLACE; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::BODY_TRACK; }
      
      virtual u8 GetMovementTracksToIgnore() const override { return (u8)AnimTrackFlag::BODY_TRACK; }
      
      // Modify default parameters (must be called before Init() to have an effect)
      void SetMaxSpeed(f32 maxSpeed_radPerSec);
      void SetAccel(f32 accel_radPerSec2)                { _accel_radPerSec2 = accel_radPerSec2; }
      void SetTolerance(const Radians& angleTol_rad);
      void SetVariability(const Radians& angleVar_rad)   { _variability = angleVar_rad; }
      
      void SetMoveEyes(bool enable) { _moveEyes = enable; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      virtual void Cleanup() override;
      
    private:
      
      bool IsBodyInPosition(const Robot& robot, Radians& currentAngle) const;
      
      bool    _inPosition = false;
      bool    _turnStarted = false;
      Radians _targetAngle;
      Radians _angleTolerance = POINT_TURN_ANGLE_TOL;
      Radians _variability = 0;
      bool    _isAbsoluteAngle;
      f32     _maxSpeed_radPerSec = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
      f32     _accel_radPerSec2 = 10.f;
      Radians _halfAngle = 0.f;
      
      bool    _wasKeepFaceAliveEnabled;
      bool    _moveEyes = true;
      AnimationStreamer::Tag _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
      
    }; // class TurnInPlaceAction


    // A simple action for drving a straight line forward or backward, without
    // using the planner
    class DriveStraightAction : public IAction
    {
    public:
      // Positive distance for forward, negative for backward.
      // Speed should be positive.
      DriveStraightAction(f32 dist_mm, f32 speed_mmps);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_STRAIGHT; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::BODY_TRACK; }
      
      virtual u8 GetMovementTracksToIgnore() const override { return (u8)AnimTrackFlag::BODY_TRACK; }
      
      void SetAccel(f32 accel_mmps2) { _accel_mmps2 = accel_mmps2; }
      void SetDecel(f32 decel_mmps2) { _decel_mmps2 = decel_mmps2; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      
      f32 _dist_mm = 0.f;
      f32 _speed_mmps  = DEFAULT_PATH_SPEED_MMPS;
      f32 _accel_mmps2 = DEFAULT_PATH_ACCEL_MMPS2;
      f32 _decel_mmps2 = DEFAULT_PATH_ACCEL_MMPS2;
      
      bool _hasStarted = false;
      
      std::string _name = "DriveStraightAction";
      
    }; // class DriveStraightAction
    
    
    class PanAndTiltAction : public IAction
    {
    public:
      // Rotate the body according to bodyPan angle and tilt the head according
      // to headTilt angle. Angles are considered relative to current robot pose
      // if isAbsolute==false.
      // If an angle is less than AngleTol, then no movement occurs but the
      // eyes will dart to look at the angle.
      PanAndTiltAction(Radians bodyPan, Radians headTilt,
                       bool isPanAbsolute, bool isTiltAbsolute);
      
      virtual const std::string& GetName() const override { return _name; }
      
      virtual RobotActionType GetType() const override { return RobotActionType::PAN_AND_TILT; }
      
      virtual u8 GetAnimTracksToDisable() const override {
        return (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK;
      }
      
      virtual u8 GetMovementTracksToIgnore() const override { return (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::BODY_TRACK; }
      
      // Modify default parameters (must be called before Init() to have an effect)
      void SetMaxPanSpeed(f32 maxSpeed_radPerSec);
      void SetPanAccel(f32 accel_radPerSec2);
      void SetPanTolerance(const Radians& angleTol_rad);
      void SetMaxTiltSpeed(f32 maxSpeed_radPerSec);
      void SetTiltAccel(f32 accel_radPerSec2);
      void SetTiltTolerance(const Radians& angleTol_rad);
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      void SetBodyPanAngle(Radians angle) { _bodyPanAngle = angle; }
      void SetHeadTiltAngle(Radians angle) { _headTiltAngle = angle; }
      
    private:
      IActionRunner*   _compoundAction;
      
      Radians _bodyPanAngle;
      Radians _headTiltAngle;
      bool    _isPanAbsolute;
      bool    _isTiltAbsolute;
      
      const f32 _kDefaultPanAngleTol  = DEG_TO_RAD(5);
      const f32 _kDefaultMaxPanSpeed  = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
      const f32 _kDefaultPanAccel     = 10.f;
      const f32 _kDefaultTiltAngleTol = DEG_TO_RAD(5);
      const f32 _kDefaultMaxTiltSpeed = 15.f;
      const f32 _kDefaultTiltAccel    = 20.f;
      
      Radians _panAngleTol            = _kDefaultPanAngleTol;
      f32     _maxPanSpeed_radPerSec  = _kDefaultMaxPanSpeed;
      f32     _panAccel_radPerSec2    = _kDefaultPanAccel;
      Radians _tiltAngleTol           = _kDefaultTiltAngleTol;
      f32     _maxTiltSpeed_radPerSec = _kDefaultMaxTiltSpeed;
      f32     _tiltAccel_radPerSec2   = _kDefaultTiltAccel;
      
      std::string _name = "PanAndTiltAction";
      
    }; // class PanAndTiltAction
    
    
    class MoveHeadToAngleAction : public IAction
    {
    public:
      MoveHeadToAngleAction(const Radians& headAngle, const Radians& tolerance = HEAD_ANGLE_TOL,
                            const Radians& variability = 0);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::MOVE_HEAD_TO_ANGLE; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (u8)AnimTrackFlag::HEAD_TRACK; }
      
      virtual u8 GetMovementTracksToIgnore() const override { return (u8)AnimTrackFlag::HEAD_TRACK; }
      
      // Modify default parameters (must be called before Init() to have an effect)
      // TODO: Use setters for variability and tolerance too
      void SetMaxSpeed(f32 maxSpeed_radPerSec)   { _maxSpeed_radPerSec = maxSpeed_radPerSec; }
      void SetAccel(f32 accel_radPerSec2)        { _accel_radPerSec2 = accel_radPerSec2; }
      void SetDuration(f32 duration_sec)         { _duration_sec = duration_sec; }
      
      // Enable/disable eye movement while turning. If hold is true, the eyes will
      // remain in their final position until the next time something moves the head.
      void SetMoveEyes(bool enable, bool hold=false) { _moveEyes = enable; _holdEyes = hold; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      virtual void Cleanup() override;
      
    private:
      
      bool IsHeadInPosition(const Robot& robot) const;
      
      Radians     _headAngle;
      Radians     _angleTolerance;
      Radians     _variability;
      
      std::string _name;
      bool        _inPosition;
      
      f32         _maxSpeed_radPerSec = 15.f;
      f32         _accel_radPerSec2   = 20.f;
      f32         _duration_sec = 0.f;
      bool        _moveEyes = true;
      bool        _holdEyes = false;
      Radians     _halfAngle;
      bool        _wasKeepFaceAliveEnabled;
      
      AnimationStreamer::Tag _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
      
      bool        _motionStarted = false;
      
    };  // class MoveHeadToAngleAction
    
    
    // Set the lift to specified height with a given tolerance. Note that settign
    // the tolerance too small will likely lead to an action timeout.
    class MoveLiftToHeightAction : public IAction
    {
    public:
      
      // Named presets:
      enum class Preset : u8 {
        LOW_DOCK,
        HIGH_DOCK,
        CARRY,
        OUT_OF_FOV // Moves to low or carry, depending on which is closer to current height
      };
      
      MoveLiftToHeightAction(const f32 height_mm, const f32 tolerance_mm = 5.f, const f32 variability = 0);
      MoveLiftToHeightAction(const Preset preset, const f32 tolerance_mm = 5.f);
      
      virtual const std::string& GetName() const override { return _name; };
      virtual RobotActionType GetType() const override { return RobotActionType::MOVE_LIFT_TO_HEIGHT; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::LIFT_TRACK; }
      
      virtual u8 GetMovementTracksToIgnore() const override { return (u8)AnimTrackFlag::LIFT_TRACK; }
      
      // how long this action should take (which, in turn, effects lift speed)
      void SetDuration(float duration_sec) { _duration = duration_sec; }
      
      void SetMaxLiftSpeed(float speedRadPerSec) { _maxLiftSpeedRadPerSec = speedRadPerSec; }
      void SetLiftAccel(float accelRadPerSec2) { _liftAccelRacPerSec2 = accelRadPerSec2; }
      
    protected:
      
      static f32 GetPresetHeight(Preset preset);
      static const std::string& GetPresetName(Preset preset);
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      
      bool IsLiftInPosition(const Robot& robot) const;
      
      f32         _height_mm;
      f32         _heightTolerance;
      f32         _variability;
      f32         _heightWithVariation;
      f32         _duration = 0.0f; // 0 means "as fast as it can"
      f32         _maxLiftSpeedRadPerSec = 10.0f;
      f32         _liftAccelRacPerSec2 = 20.0f;
      
      std::string _name;
      bool        _inPosition;
      bool        _motionStarted = false;
      
    }; // class MoveLiftToHeightAction
    

    class PlaceObjectOnGroundAction : public IAction
    {
    public:
      
      PlaceObjectOnGroundAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::LIFT_TRACK; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker = nullptr;
      IActionRunner*              _faceAndVerifyAction = nullptr;
      ObjectInteractionResult     _interactionResult = ObjectInteractionResult::INCOMPLETE;
      
    }; // class PlaceObjectOnGroundAction
    
    
    // Common compound action
    // @param placementPose    - The pose in which the carried object should be placed.
    // @param useExactRotation - If true, then the carried object is placed in the exact
    //                           6D pose represented by placement pose. Otherwise,
    //                           x,y and general axis alignment with placementPose rotation
    //                           are the only constraints.
    class PlaceObjectOnGroundAtPoseAction : public CompoundActionSequential
    {
    public:
      PlaceObjectOnGroundAtPoseAction(const Pose3d& placementPose,
                                      const PathMotionProfile motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                      const bool useExactRotation = false,
                                      const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
    };
    
    
    // This is just a selector for AscendOrDescendRampAction or
    // CrossBridgeAction, depending on the object's type.
    class TraverseObjectAction : public IActionRunner
    {
    public:
      TraverseObjectAction(ObjectID objectID, const bool useManualSpeed);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::TRAVERSE_OBJECT; }
      
      void SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2);
      
    protected:
      
      // Update will just call the chosenAction's implementation
      virtual ActionResult UpdateInternal() override;
      virtual void Reset() override { }
      
      ObjectID       _objectID;
      IActionRunner* _chosenAction = nullptr;
      f32            _speed_mmps;
      f32            _accel_mmps2;
      bool           _useManualSpeed;
      
    }; // class TraverseObjectAction
    
    
    // Tilt head and rotate body to face the given pose.
    // Use angles specified at construction to control the body rotation.
    class FacePoseAction : public PanAndTiltAction
    {
    public:
      // Note that the rotation information in pose will be ignored
      FacePoseAction(const Pose3d& pose, Radians maxTurnAngle);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FACE_POSE; }
      
    protected:
      virtual ActionResult Init() override;
      
      FacePoseAction(Radians maxTurnAngle);
      
      void SetPose(const Pose3d& pose);
      virtual Radians GetHeadAngle(f32 heightDiff);
      
    private:
      Pose3d    _poseWrtRobot;
      bool      _isPoseSet;
      Radians   _maxTurnAngle;
      
    }; // class FacePoseAction
    
    
    // Tilt head and rotate body to face the specified (marker on an) object.
    // Use angles specified at construction to control the body rotation.
    class FaceObjectAction : public FacePoseAction
    {
    public:
      // If facing the object requires less than turnAngleTol turn, then no
      // turn is performed. If a turn greater than maxTurnAngle is required,
      // the action fails. For angles in between, the robot will first turn
      // to face the object, then tilt its head. To disallow turning, set
      // maxTurnAngle to zero.
      
      FaceObjectAction(ObjectID objectID,
                       Radians maxTurnAngle,
                       bool visuallyVerifyWhenDone = false,
                       bool headTrackWhenDone = false);
      
      FaceObjectAction(ObjectID objectID,
                       Vision::Marker::Code whichCode,
                       Radians maxTurnAngle,
                       bool visuallyVerifyWhenDone = false,
                       bool headTrackWhenDone = false);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FACE_OBJECT; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      virtual Radians GetHeadAngle(f32 heightDiff) override;
      
      bool                 _facePoseCompoundActionDone;
      
      IActionRunner*    _visuallyVerifyAction;
      
      ObjectID             _objectID;
      Vision::Marker::Code _whichCode;
      bool                 _visuallyVerifyWhenDone;
      bool                 _headTrackWhenDone;
      
    }; // FaceObjectAction
    
    
    // Verify that an object exists by facing tilting the head to face its
    // last-known pose and verify that we can still see it. Optionally, you can
    // also require that a specific marker be seen as well.
    class VisuallyVerifyObjectAction : public IAction
    {
    public:
      VisuallyVerifyObjectAction(ObjectID objectID,
                                 Vision::Marker::Code whichCode = Vision::Marker::ANY_CODE);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::VISUALLY_VERIFY_OBJECT; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::HEAD_TRACK; }
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      // Max amount of time to wait before verifying after moving head that we are
      // indeed seeing the object/marker we expect.
      // TODO: Can this default be reduced?
      virtual f32 GetWaitToVerifyTime() const { return 0.25f; }
      
      ObjectID                _objectID;
      Vision::Marker::Code    _whichCode;
      f32                     _waitToVerifyTime;
      bool                    _objectSeen;
      bool                    _markerSeen;
      IActionRunner*          _moveLiftToHeightAction;
      bool                    _moveLiftToHeightActionDone;
      Signal::SmartHandle     _observedObjectHandle;
      
    }; // class VisuallyVerifyObjectAction
    
    
    // Waits for a specified amount of time in seconds, from the time the action
    // is begun. Returns RUNNING while waiting and SUCCESS when the time has
    // elapsed.
    class WaitAction : public IAction
    {
    public:
      WaitAction(f32 waitTimeInSeconds);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::WAIT; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      f32         _waitTimeInSeconds;
      f32         _doneTimeInSeconds;
      std::string _name;
      
    };
    
    
    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(const std::string& animName,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::PLAY_ANIMATION; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      virtual void Cleanup() override;
      
      std::string               _animName;
      std::string               _name;
      u32                       _numLoops;
      bool                      _startedPlaying;
      bool                      _stoppedPlaying;
      bool                      _wasAborted;
      AnimationStreamer::Tag    _animTag = AnimationStreamer::NotAnimatingTag;
      bool                      _interruptRunning;
      
      // For responding to AnimationStarted and AnimationEnded events
      Signal::SmartHandle _startSignalHandle;
      Signal::SmartHandle _endSignalHandle;
      Signal::SmartHandle _abortSignalHandle;
      
    private:
      // For handling playing an altered copy of an animation
      std::unique_ptr<Animation> _alteredAnimation;
      
      bool NeedsAlteredAnimation(Robot& robot) const;
      
    }; // class PlayAnimationAction

    
    class PlayAnimationGroupAction : public PlayAnimationAction
    {
    public:
      explicit PlayAnimationGroupAction(const std::string& animGroupName,
                                        u32 numLoops = 1,
                                        bool interruptRunning = true);
      
    protected:
      virtual ActionResult Init() override;
      
      std::string   _animGroupName;
      
    }; // class PlayAnimationGroupAction
    
    
    class DeviceAudioAction : public IAction
    {
    public:
      // Play Audio Event
      // TODO: Add bool to set if caller want's to block "wait" until audio is completed
      DeviceAudioAction(const Audio::EventType event,
                        const Audio::GameObjectType gameObj,
                        const bool waitUntilDone = false);
      
      // Stop All Events on Game Object, pass in Invalid to stop all audio
      DeviceAudioAction(const Audio::GameObjectType gameObj);
      
      // Change Music state
      DeviceAudioAction(const Audio::MusicGroupStates state);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::DEVICE_AUDIO; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      enum class AudioActionType : uint8_t {
        Event = 0,
        StopEvents,
        SetState,
      };
      
      AudioActionType           _actionType;
      std::string               _name;
      bool                      _isCompleted    = false;
      bool                      _waitUntilDone  = false;
      Audio::EventType          _event          = Audio::EventType::Invalid;
      Audio::GameObjectType     _gameObj        = Audio::GameObjectType::Invalid;
      Audio::GameStateGroupType _stateGroup     = Audio::GameStateGroupType::Invalid;
      Audio::GameStateType      _state          = Audio::GameStateType::Invalid;
      
    }; // class PlayAudioAction
  }
}

#endif /* ANKI_COZMO_BASIC_ACTIONS_H */
