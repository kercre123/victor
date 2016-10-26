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
#include "anki/vision/basestation/faceIdTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/toolCodes.h"
#include "clad/types/visionModes.h"
#include "util/bitFlags/bitFlags.h"
#include "util/helpers/templateHelpers.h"

#include <vector>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class VisuallyVerifyObjectAction;

    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public IAction
    {
    public:
      TurnInPlaceAction(Robot& robot, const Radians& angle, const bool isAbsolute);
      virtual ~TurnInPlaceAction();
      
      // Modify default parameters (must be called before Init() to have an effect)
      void SetMaxSpeed(f32 maxSpeed_radPerSec);
      void SetAccel(f32 accel_radPerSec2);
      void SetTolerance(const Radians& angleTol_rad);
      void SetVariability(const Radians& angleVar_rad)   { _variability = angleVar_rad; }
      
      void SetMoveEyes(bool enable) { _moveEyes = enable; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      
      bool IsBodyInPosition(Radians& currentAngle) const;
      Result SendSetBodyAngle() const;
      
      const f32 _kDefaultSpeed     = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
      const f32 _kDefaultAccel     = 10.f;
      
      bool    _inPosition = false;
      bool    _turnStarted = false;
      const Radians _requestedTargetAngle;
      Radians _currentAngle;
      Radians _currentTargetAngle, _initialAngle, _halfAngle;
      Pose3d  _targetPose, _initialPose;
      Radians _angleTolerance = POINT_TURN_ANGLE_TOL;
      Radians _variability = 0;
      bool    _isAbsoluteAngle;
      f32     _maxSpeed_radPerSec = _kDefaultSpeed;
      f32     _accel_radPerSec2 = _kDefaultAccel;
      
      bool    _wasKeepFaceAliveEnabled;
      bool    _moveEyes = true;
      AnimationStreamer::Tag _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
      
    }; // class TurnInPlaceAction

    // A simple compound action which is useful for identifying blocks that are close
    // to Cozmo's current frame of view.  Cozmo drives backwards slightly, looks left and right
    // and up and down slightly to identify blocks that may be slightly outside the camera
    // optionally an objectID can be passed in for the action to complete immediately on finding the object
    class SearchForNearbyObjectAction : public IAction
    {
    public:
      SearchForNearbyObjectAction(Robot& robot, const ObjectID& desiredObjectID = ObjectID());
      virtual ~SearchForNearbyObjectAction();

      void SetSearchAngle(f32 minSearchAngle_rads, f32 maxSearchAngle_rads);
      void SetSearchWaitTime(f32 minWaitTime_s, f32 maxWaitTime_s);

    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;

    private:
      CompoundActionSequential _compoundAction;
      ObjectID _desiredObjectID;

      f32 _minWaitTime_s = 0.8f;
      f32 _maxWaitTime_s = 1.2f;
      f32 _minSearchAngle_rads = DEG_TO_RAD(15.0f);
      f32 _maxSearchAngle_rads = DEG_TO_RAD(20.0f);
      bool _shouldPopIdle = false;
    };

    // A simple action for drving a straight line forward or backward, without
    // using the planner
    class DriveStraightAction : public IAction
    {
    public:
      // Positive distance for forward, negative for backward.
      // Speed should be positive.
      DriveStraightAction(Robot& robot, f32 dist_mm, f32 speed_mmps, bool shouldPlayAnimation = true);
      virtual ~DriveStraightAction();
      
      void SetAccel(f32 accel_mmps2) { _accel_mmps2 = accel_mmps2; }
      void SetDecel(f32 decel_mmps2) { _decel_mmps2 = decel_mmps2; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      
      f32 _dist_mm = 0.f;
      f32 _speed_mmps  = DEFAULT_PATH_MOTION_PROFILE.speed_mmps;
      f32 _accel_mmps2 = DEFAULT_PATH_MOTION_PROFILE.accel_mmps2;
      f32 _decel_mmps2 = DEFAULT_PATH_MOTION_PROFILE.decel_mmps2;
      
      bool _hasStarted = false;
      
      bool _shouldPlayDrivingAnimation = true;
      
    }; // class DriveStraightAction
    
    
    class PanAndTiltAction : public IAction
    {
    public:
      // Rotate the body according to bodyPan angle and tilt the head according
      // to headTilt angle. Angles are considered relative to current robot pose
      // if isAbsolute==false.
      // If an angle is less than AngleTol, then no movement occurs but the
      // eyes will dart to look at the angle.
      PanAndTiltAction(Robot& robot, Radians bodyPan, Radians headTilt,
                       bool isPanAbsolute, bool isTiltAbsolute);
      virtual ~PanAndTiltAction();
      
      // Modify default parameters (must be called before Init() to have an effect)
      void SetMaxPanSpeed(f32 maxSpeed_radPerSec);
      void SetPanAccel(f32 accel_radPerSec2);
      void SetPanTolerance(const Radians& angleTol_rad);
      void SetMaxTiltSpeed(f32 maxSpeed_radPerSec);
      void SetTiltAccel(f32 accel_radPerSec2);
      void SetTiltTolerance(const Radians& angleTol_rad);
      void SetMoveEyes(bool enable) { _moveEyes = enable; }
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      void SetBodyPanAngle(Radians angle) { _bodyPanAngle = angle; }
      void SetHeadTiltAngle(Radians angle) { _headTiltAngle = angle; }
      
    private:
      CompoundActionParallel _compoundAction;
      
      Radians _bodyPanAngle   = 0.f;
      Radians _headTiltAngle  = 0.f;
      bool    _isPanAbsolute  = false;
      bool    _isTiltAbsolute = false;
      bool    _moveEyes       = true;
      
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
      
    }; // class PanAndTiltAction
    
  
  
    class CalibrateMotorAction : public IAction
    {
    public:
      CalibrateMotorAction(Robot& robot,
                           bool calibrateHead,
                           bool calibrateLift);
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      bool _calibHead;
      bool _calibLift;
      
      bool _headCalibStarted;
      bool _liftCalibStarted;
    };
  
      
  
    class MoveHeadToAngleAction : public IAction
    {
    public:
      enum class Preset : u8 {
        GROUND_PLANE_VISIBLE      // at this head angle, the whole ground plane (or the max amount) will be visible
      };
    
      MoveHeadToAngleAction(Robot& robot,
                            const Radians& headAngle,
                            const Radians& tolerance = HEAD_ANGLE_TOL,
                            const Radians& variability = 0);

      MoveHeadToAngleAction(Robot& robot,
                            const Preset preset,
                            const Radians& tolerance = HEAD_ANGLE_TOL,
                            const Radians& variability = 0);
      
      virtual ~MoveHeadToAngleAction();
      
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
      
    private:
    
      static f32 GetPresetHeadAngle(Preset preset);
      static const char* GetPresetName(Preset preset);
      
      bool IsHeadInPosition() const;
      
      Radians     _headAngle;
      Radians     _angleTolerance;
      Radians     _variability;
      
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
      
      MoveLiftToHeightAction(Robot& robot, const f32 height_mm,
                             const f32 tolerance_mm = 5.f, const f32 variability = 0);
      MoveLiftToHeightAction(Robot& robot, const Preset preset, const f32 tolerance_mm = 5.f);
      
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
      
      bool IsLiftInPosition() const;
      
      f32         _height_mm;
      f32         _heightTolerance;
      f32         _variability;
      f32         _heightWithVariation;
      f32         _duration = 0.0f; // 0 means "as fast as it can"
      f32         _maxLiftSpeedRadPerSec = 10.0f;
      f32         _liftAccelRacPerSec2 = 20.0f;
      
      bool        _inPosition;
      bool        _motionStarted = false;
      
    }; // class MoveLiftToHeightAction
    
    
    // This is just a selector for AscendOrDescendRampAction or
    // CrossBridgeAction, depending on the object's type.
    class TraverseObjectAction : public IActionRunner
    {
    public:
      TraverseObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed);
      virtual ~TraverseObjectAction()
      {
        if(_chosenAction != nullptr)
        {
          _chosenAction->PrepForCompletion();
        }
        Util::SafeDelete(_chosenAction);
      }
      
      void SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2);
      
    protected:
      
      // Update will just call the chosenAction's implementation
      virtual ActionResult UpdateInternal() override;
      virtual void Reset(bool shouldUnlockTracks = true) override { }
      
      ObjectID       _objectID;
      IActionRunner* _chosenAction = nullptr;
      f32            _speed_mmps;
      f32            _accel_mmps2;
      f32            _decel_mmps2;
      bool           _useManualSpeed;
      
    }; // class TraverseObjectAction
    
    
    // Tilt head and rotate body to face the given pose.
    // Use angles specified at construction to control the body rotation.
    class TurnTowardsPoseAction : public PanAndTiltAction
    {
    public:
      // Note that the rotation information in pose will be ignored
      TurnTowardsPoseAction(Robot& robot, const Pose3d& pose, Radians maxTurnAngle);
      
      void SetMaxTurnAngle(Radians angle) { _maxTurnAngle = angle; }

      // compute the turn angles. Can be useful to check what will happen before this action
      // executes. Arguments are translations with respect to the robot
      static Radians GetAbsoluteHeadAngleToLookAtPose(const Point3f& translationWrtRobot);
      static Radians GetRelativeBodyAngleToLookAtPose(const Point3f& translationWrtRobot);
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      TurnTowardsPoseAction(Robot& robot, Radians maxTurnAngle);
      
      void SetPose(const Pose3d& pose);
      
      Pose3d    _poseWrtRobot;
      
    private:
      Radians   _maxTurnAngle;
      bool      _isPoseSet   = false;
      bool      _nothingToDo = false;
      
      static constexpr f32 kHeadAngleDistBias_rad = DEG_TO_RAD_F32(5);
      static constexpr f32 kHeadAngleHeightBias_rad = DEG_TO_RAD_F32(7.5);      
      
    }; // class TurnTowardsPoseAction
  
  
    // Wait for some number of images to be processed by the robot.
    // Optionally specify to only start counting images after a given timestamp.
    class WaitForImagesAction : public IAction
    {
    public:
      
      // VisionMode indicates the vision mode(s) that this action wants to wait for, for numFrames instances. VisionMode::Count means any
      WaitForImagesAction(Robot& robot, u32 numFrames, VisionMode visionMode = VisionMode::Count, TimeStamp_t afterTimeStamp = 0);
      virtual ~WaitForImagesAction() { }
      
      virtual f32 GetTimeoutInSeconds() const override { return std::numeric_limits<f32>::max(); }
      
    protected:
      
      virtual ActionResult Init() override;
      
      virtual ActionResult CheckIfDone() override;
      
    private:
      u32 _numFramesToWaitFor;
      TimeStamp_t _afterTimeStamp;
      
      Signal::SmartHandle             _imageProcSignalHandle;
      VisionMode                      _visionMode = VisionMode::Count;
      u32                             _numModeFramesSeen = 0;
      
    }; // WaitForImagesAction()
  
    // Tilt head and rotate body to face the specified (marker on an) object.
    // Use angles specified at construction to control the body rotation.
    class TurnTowardsObjectAction : public TurnTowardsPoseAction
    {
    public:
      // If facing the object requires less than turnAngleTol turn, then no
      // turn is performed. If a turn greater than maxTurnAngle is required,
      // the action does nothing but succeeds. For angles in between, the robot
      // will first turn to face the object, then tilt its head. To disallow turning,
      // set maxTurnAngle to zero.
      
      TurnTowardsObjectAction(Robot& robot,
                              ObjectID objectID,
                              Radians maxTurnAngle,
                              bool visuallyVerifyWhenDone = false,
                              bool headTrackWhenDone = false);
      
      TurnTowardsObjectAction(Robot& robot,
                              ObjectID objectID,
                              Vision::Marker::Code whichCode,
                              Radians maxTurnAngle,
                              bool visuallyVerifyWhenDone = false,
                              bool headTrackWhenDone = false);
      
      virtual ~TurnTowardsObjectAction();

      // usually, an object ID should be passed in in the constructor, but this function can be called to
      // specify an object pointer which may live outside of blockworld
      void UseCustomObject(ObservableObject* objectPtr);
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      void ShouldDoRefinedTurn(const bool tf) { _doRefinedTurn = tf; }
      void SetRefinedTurnAngleTol(const f32 tol) { _refinedTurnAngleTol_rad = tol; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;

      bool                       _facePoseCompoundActionDone = false;
      
      VisuallyVerifyObjectAction*_visuallyVerifyAction = nullptr;
      bool                       _refinedTurnTowardsDone = false;
      
      ObjectID                   _objectID;
      ObservableObject*          _objectPtr = nullptr;
      Vision::Marker::Code       _whichCode;
      bool                       _headTrackWhenDone;
      bool                       _doRefinedTurn = true;
      f32                        _refinedTurnAngleTol_rad = DEG_TO_RAD_F32(5);
      
    }; // TurnTowardsObjectAction
    
    
    // Turn towards the last known face pose. Note that this action "succeeds" without doing
    // anything if there is no face (unless requireFace is set to true)
    // If a face is seen after we stop turning, "fine tune" the turn a bit and
    // say the face's name if we recognize it (and sayName=true).
    class TurnTowardsFaceAction : public TurnTowardsPoseAction
    {
    public:
      TurnTowardsFaceAction(Robot& robot, Vision::FaceID_t faceID, Radians maxTurnAngle = PI_F, bool sayName = false);
      virtual ~TurnTowardsFaceAction();
      
      // Set the maximum number of frames we are will to wait to see a face after
      // the initial blind turn to the last face pose.
      void SetMaxFramesToWait(u32 N) { _maxFramesToWait = N; }

      // Sets the animation trigger to use to say the name. Only valid if sayName was true
      void SetSayNameAnimationTrigger(AnimationTrigger trigger);
      
      // Sets the backup animation to play if the name is not known, but there is a confirmed face. Only valid
      // if sayName is true (this is because we are trying to use an animation to say the name, but if we
      // don't have a name, we want to use this animation instead)
      void SetNoNameAnimationTrigger(AnimationTrigger trigger);

      // Sets whether or not we require a face. Default is false (it will play animations and return success
      // even if no face is found). If set to true and no face is found, the action will fail with
      // FAILURE_ABORT and no animations will be played
      void SetRequireFaceConfirmation(bool isRequired) { _requireFaceConfirmation = isRequired; }
      
      // Template for all events we subscribe to
      template<typename T>
      void HandleMessage(const T& msg);
      
    protected:

      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;

    private:
      enum class State : u8 {
        Turning,
        WaitingForFace,
        FineTuning,
        SayingName, // saying name, or playing noNameAnimTrigger
      };
      
      Vision::FaceID_t  _faceID                  = Vision::UnknownFaceID;
      IActionRunner*    _action                  = nullptr;
      f32               _closestDistSq           = std::numeric_limits<f32>::max();
      u32               _maxFramesToWait         = 10;
      Vision::FaceID_t  _obsFaceID               = Vision::UnknownFaceID;
      State             _state                   = State::Turning;
      bool              _sayName                 = false;
      bool              _tracksLocked            = false;
      bool              _requireFaceConfirmation = false;
      AnimationTrigger  _nameAnimTrigger         = AnimationTrigger::Count;
      AnimationTrigger  _noNameAnimTrigger       = AnimationTrigger::Count;

      
      std::vector<Signal::SmartHandle> _signalHandles;

      void CreateFineTuneAction();
      void SetAction(IActionRunner* action);
      
    }; // TurnTowardsLastFacePoseAction

  
    class TurnTowardsLastFacePoseAction : public TurnTowardsFaceAction
    {
    public:
      TurnTowardsLastFacePoseAction(Robot& robot, Radians maxTurnAngle = PI_F, bool sayName = false)
      : TurnTowardsFaceAction(robot, Vision::UnknownFaceID, maxTurnAngle, sayName)
      {
        
      }
    };
  
    // Turn towards the last face before or after another action
    class TurnTowardsFaceWrapperAction : public CompoundActionSequential
    {
    public:

      // Create a wrapper around the given action which looks towards a face before and/or after (default
      // before) the action. This takes ownership of action, the pointer should not be used after this call
      TurnTowardsFaceWrapperAction(Robot& robot,
                                   IActionRunner* action,
                                   bool turnBeforeAction = true,
                                   bool turnAfterAction = false,
                                   Radians maxTurnAngle = PI_F,
                                   bool sayName = false);
    };
    
    
    // Waits for a specified amount of time in seconds, from the time the action
    // is begun. Returns RUNNING while waiting and SUCCESS when the time has
    // elapsed.
    class WaitAction : public IAction
    {
    public:
      WaitAction(Robot& robot, f32 waitTimeInSeconds);
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      f32         _waitTimeInSeconds;
      f32         _doneTimeInSeconds;
      
    };

  
    // Dummy action that just never finishes, can be useful for testing or holding the queue
    class HangAction : public IAction
    {
    public:
      HangAction(Robot& robot) : IAction(robot,
                                         "Hang",
                                         RobotActionType::HANG,
                                         (u8)AnimTrackFlag::NO_TRACKS) {}

      virtual f32 GetTimeoutInSeconds() const override { return std::numeric_limits<f32>::max(); }
      
    protected:
      
      virtual ActionResult Init() override { return ActionResult::SUCCESS; }
      virtual ActionResult CheckIfDone() override { return ActionResult::RUNNING; }

    };

    class WaitForLambdaAction : public IAction
    {
    public:
      WaitForLambdaAction(Robot& robot, std::function<bool(Robot&)> lambda)
        : IAction(robot,
                  "WaitForLambda",
                  RobotActionType::WAIT_FOR_LAMBDA,
                  (u8)AnimTrackFlag::NO_TRACKS)
        , _lambda(lambda)
        {
        }
      virtual ~WaitForLambdaAction() { }

      virtual f32 GetTimeoutInSeconds() const override { return std::numeric_limits<f32>::max(); }
      
    protected:
      
      virtual ActionResult Init() override { return ActionResult::SUCCESS; }
      virtual ActionResult CheckIfDone() override {
        if( _lambda(_robot) ){
          return ActionResult::SUCCESS;
        }
        else {
          return ActionResult::RUNNING;
        }
      }

    private:
      
      std::function<bool(Robot&)> _lambda;

    };
    
    
    class ReadToolCodeAction : public IAction
    {
    public:
      
      ReadToolCodeAction(Robot& robot, bool doCalibration = false);
      virtual ~ReadToolCodeAction();
      
      virtual f32 GetTimeoutInSeconds() const override { return 5.f; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      
      std::string       _name = "ReadToolCode";
      bool              _doCalibration = false;
      ToolCodeInfo      _toolCodeInfo;
      
      CompoundActionParallel _headAndLiftDownAction;
      
      //const TimeStamp_t kRequiredStillTime_ms    = 500;
  
      enum class State : u8 {
        WaitingToGetInPosition,
        WaitingForRead,
        ReadCompleted
      } _state;
  
      // Handler for the tool code being read
      Signal::SmartHandle        _toolReadSignalHandle;
      
    }; // class ReadToolCodeAction
    
} // namespace Cozmo
} // namespace Anki

#endif /* ANKI_COZMO_BASIC_ACTIONS_H */
