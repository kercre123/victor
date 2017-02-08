/**
 * File: basicActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements basic cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actions/basicActions.h"

#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/visionModesHelpers.h"

namespace Anki {
  
  namespace Cozmo {
    
    TurnInPlaceAction::TurnInPlaceAction(Robot& robot, const Radians& angle, const bool isAbsolute)
    : IAction(robot,
              "TurnInPlace",
              RobotActionType::TURN_IN_PLACE,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _requestedTargetAngle(angle)
    , _isAbsoluteAngle(isAbsolute)
    {
      
    }
    
    TurnInPlaceAction::~TurnInPlaceAction()
    {
      if(_moveEyes)
      {
        // Make sure eye shift gets removed no matter what
        if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag) {
          _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
      }

      if( IsRunning() ) {
        // stop the robot turning if the action is destroyed while running
        _robot.GetMoveComponent().StopAllMotors();
      }
    }
    
    void TurnInPlaceAction::SetMaxSpeed(f32 maxSpeed_radPerSec)
    {
      if (std::fabsf(maxSpeed_radPerSec) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        PRINT_NAMED_WARNING("TurnInPlaceAction.SetMaxSpeed.SpeedExceedsLimit",
                            "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
                            RAD_TO_DEG(maxSpeed_radPerSec), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _maxSpeed_radPerSec = std::copysign(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, maxSpeed_radPerSec);
      } else if (maxSpeed_radPerSec == 0) {
        _maxSpeed_radPerSec = _kDefaultSpeed;
      } else {
        _maxSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void TurnInPlaceAction::SetAccel(f32 accel_radPerSec2) {
      if (accel_radPerSec2 == 0) {
        _accel_radPerSec2 = _kDefaultAccel;
      } else {
        _accel_radPerSec2 = accel_radPerSec2;
      }
    }
    
    
    void TurnInPlaceAction::SetTolerance(const Radians& angleTol_rad)
    {
      _angleTolerance = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _angleTolerance.ToFloat() < POINT_TURN_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("TurnInPlaceAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            _angleTolerance.getDegrees(),
                            RAD_TO_DEG(POINT_TURN_ANGLE_TOL));
        _angleTolerance = POINT_TURN_ANGLE_TOL;
      }
    }
    
    Result TurnInPlaceAction::SendSetBodyAngle() const
    {
      RobotInterface::SetBodyAngle setBodyAngle;
      setBodyAngle.angle_rad             = _currentTargetAngle.ToFloat();
      setBodyAngle.max_speed_rad_per_sec = _maxSpeed_radPerSec;
      setBodyAngle.accel_rad_per_sec2    = _accel_radPerSec2;
      setBodyAngle.angle_tolerance       = _angleTolerance.ToFloat();
      
      Result result = _robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle));
      return result;
    }
    
    ActionResult TurnInPlaceAction::Init()
    {
      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose, taking into account the current driveCenter offset
      Radians currentHeading = 0;
      if (!_isAbsoluteAngle) {
        currentHeading = _robot.GetPose().GetRotationAngle<'Z'>();
      }
      
      // the new angle is, current + requested - [what we have done so far for retries, 0 for first time]
      const float doneSoFar = (_currentAngle-_initialAngle).ToFloat();
      Radians newAngle;
      newAngle = currentHeading + _requestedTargetAngle - doneSoFar;
      if(_variability != 0) {
        newAngle += GetRNG().RandDblInRange(-_variability.ToDouble(),
                                            _variability.ToDouble());
      }
      
      Pose3d dcPose = _robot.GetDriveCenterPose();
      dcPose.SetRotation(newAngle, Z_AXIS_3D());
      _robot.ComputeOriginPose(dcPose, _targetPose);
      
      _targetPose.SetParent(_robot.GetWorldOrigin());
      _initialPose = _robot.GetPose();
      
      DEV_ASSERT(_robot.GetPose().GetParent() == _robot.GetWorldOrigin(), "TurnInPlaceAction.Init.RobotOriginMismatch");
      
      _initialAngle = _initialPose.GetRotation().GetAngleAroundZaxis();
      _currentAngle = _initialAngle;
      _currentTargetAngle = _targetPose.GetRotation().GetAngleAroundZaxis();
      
      Radians currentAngle;
      _inPosition = IsBodyInPosition(currentAngle);
      _turnStarted = false;
      
      if(!_inPosition) {

        if(RESULT_OK != SendSetBodyAngle()) {
          return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
        }
        
        if(_moveEyes)
        {
          // Remove any existing eye dart due to keeping face alive, since we're
          // doing our own
          _robot.GetAnimationStreamer().RemoveKeepAliveEyeDart(IKeyFrame::SAMPLE_LENGTH_MS);
          
          // Store half the total difference so we know when to remove eye shift
          _halfAngle = 0.5f*(_currentTargetAngle - _initialAngle).getAbsoluteVal();
          
          // Move the eyes (only if not in position)
          // Note: assuming screen is about the same x distance from the neck joint as the head cam
          Radians angleDiff = _currentTargetAngle - currentAngle;
          
          // Clip angleDiff to 89 degrees to prevent unintended behavior due to tangent
          if(angleDiff.getDegrees() > 89)
          {
            angleDiff = DEG_TO_RAD(89);
          }
          else if(angleDiff.getDegrees() < -89)
          {
            angleDiff = DEG_TO_RAD(-89);
          }
          
          f32 x_mm = std::tan(angleDiff.ToFloat()) * HEAD_CAM_POSITION[0];
          const f32 xPixShift = x_mm * (static_cast<f32>(ProceduralFace::WIDTH) / (4*SCREEN_SIZE[0]));
          _robot.ShiftEyes(_eyeShiftTag, xPixShift, 0, 4*IKeyFrame::SAMPLE_LENGTH_MS, "TurnInPlaceEyeDart");
        }
      }
      
      return ActionResult::SUCCESS;
    }
    
    bool TurnInPlaceAction::IsBodyInPosition(Radians& currentAngle) const
    {
      currentAngle = _robot.GetPose().GetRotation().GetAngleAroundZaxis();
      const bool inPosition = currentAngle.IsNear(_currentTargetAngle, _angleTolerance.ToFloat() + Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
      return inPosition && !_robot.GetMoveComponent().AreWheelsMoving();
    }
    
    ActionResult TurnInPlaceAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      // Make sure world origin didn't change mid-turn. If it did: update the initial/target/half
      // poses and angles to be in the current coordinate frame
      if(_targetPose.GetParent() != _robot.GetWorldOrigin())
      {
        if(!_targetPose.GetWithRespectTo(*_robot.GetWorldOrigin(), _targetPose))
        {
          PRINT_CH_INFO("Actions", "TurnInPlaceAction.CheckIfDone.WorldOriginUpdateFail",
                        "Could not get target pose w.r.t. current world origin");
          return ActionResult::BAD_POSE;
        }
        
        _currentTargetAngle = _targetPose.GetRotation().GetAngleAroundZaxis();
        
        // Also need to update the angle being used by the steering controller
        // on the robot:
        if(RESULT_OK != SendSetBodyAngle()) {
          return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
        }
        
        if(_moveEyes)
        {
          if(!_initialPose.GetWithRespectTo(*_robot.GetWorldOrigin(), _initialPose)) {
            // If we got target pose w.r.t. world origin, we should be able to get initial pose too!
            PRINT_NAMED_WARNING("TurnInPlaceAction.CheckIfDone.WorldOriginUpdateFail",
                                "Could not get initial pose w.r.t. current world origin");
          }
          else
          {
            _initialAngle = _initialPose.GetRotation().GetAngleAroundZaxis();
            
            // Store half the total difference so we know when to remove eye shift
            _halfAngle = 0.5f*(_currentTargetAngle - _initialAngle).getAbsoluteVal();
          }
        }
        
        PRINT_CH_INFO("Actions", "TurnInPlaceAction.CheckIfDone.WorldOriginChanged",
                      "New target angle = %.1fdeg", _currentTargetAngle.getDegrees());
      }
      
      if(!_inPosition) {
        _inPosition = IsBodyInPosition(_currentAngle);
        
      }
      
      // When we've turned at least halfway, remove eye dart
      if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag) {
        if(_inPosition || _currentAngle.IsNear(_currentTargetAngle, _halfAngle))
        {
          PRINT_CH_DEBUG("Actions", "TurnInPlaceAction.CheckIfDone.RemovingEyeShift",
                         "Currently at %.1fdeg, on the way to %.1fdeg, within "
                         "half angle of %.1fdeg", _currentAngle.getDegrees(),
                         _currentTargetAngle.getDegrees(), _halfAngle.getDegrees());
          _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
      }

      if( _robot.GetMoveComponent().AreWheelsMoving()) {
        _turnStarted = true;
      }
      
      // Wait to get a state message back from the physical robot saying its body
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(_inPosition) {
        result = ActionResult::SUCCESS;
        PRINT_CH_INFO("Actions", "TurnInPlaceAction.CheckIfDone.ReachedAngle",
                      "[%d] Reached angle: %.1fdeg vs. %.1fdeg(+/-%.1f) (tol: %f) (pfid: %d). WheelsMoving=%s",
                      GetTag(),
                      _currentAngle.getDegrees(),
                      _currentTargetAngle.getDegrees(),
                      _variability.getDegrees(),
                      _angleTolerance.getDegrees(),
                      _robot.GetPoseFrameID(),
                      (_robot.GetMoveComponent().AreWheelsMoving() ? "Yes" : "No"));
      } else {
        // Don't spam "in position" messages
        PRINT_PERIODIC_CH_DEBUG(10, "Actions", "TurnInPlaceAction.CheckIfDone.AngleNotReached",
                                "[%d] Waiting for body to reach angle: %.1fdeg vs. %.1fdeg(+/-%.1f) (tol: %f) (pfid: %d)",
                                GetTag(),
                                _currentAngle.getDegrees(),
                                _currentTargetAngle.getDegrees(),
                                _variability.getDegrees(),
                                _angleTolerance.getDegrees(),
                                _robot.GetPoseFrameID());
        

        if( _turnStarted && !_robot.GetMoveComponent().AreWheelsMoving()) {
          PRINT_NAMED_WARNING("TurnInPlaceAction.StoppedMakingProgress",
                              "[%d] giving up since we stopped moving",
                              GetTag());
          result = ActionResult::MOTOR_STOPPED_MAKING_PROGRESS;
        }
      }
      
      return result;
    }

#pragma mark ---- SearchForNearbyObjectAction ----

    SearchForNearbyObjectAction::SearchForNearbyObjectAction(Robot& robot,
                                                             const ObjectID& desiredObjectID,
                                                             f32 backupDistance_mm,
                                                             f32 backupSpeed_mms,
                                                             f32 headAngle_rad)
    : IAction(robot,
              "SearchForNearbyObjectAction",
              RobotActionType::SEARCH_FOR_NEARBY_OBJECT,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _compoundAction(robot)
    , _desiredObjectID(desiredObjectID)
    , _backupDistance_mm(backupDistance_mm)
    , _backupSpeed_mms(backupSpeed_mms)
    , _headAngle_rad(headAngle_rad)
    {

    }
  
    SearchForNearbyObjectAction::~SearchForNearbyObjectAction()
    {
      if( _shouldPopIdle ) {
        _robot.GetAnimationStreamer().PopIdleAnimation();
        _shouldPopIdle = false;
      }
      _compoundAction.PrepForCompletion();
    }
  
    void SearchForNearbyObjectAction::SetSearchAngle(f32 minSearchAngle_rads, f32 maxSearchAngle_rads)
    {
      _minSearchAngle_rads = minSearchAngle_rads;
      _maxSearchAngle_rads = maxSearchAngle_rads;
    }
  
    void SearchForNearbyObjectAction::SetSearchWaitTime(f32 minWaitTime_s, f32 maxWaitTime_s)
    {
      _minWaitTime_s = minWaitTime_s;
      _maxWaitTime_s = maxWaitTime_s;
    }

    ActionResult SearchForNearbyObjectAction::Init()
    {
      // Incase we are re-running this action
      _compoundAction.ClearActions();
      _compoundAction.EnableMessageDisplay(IsMessageDisplayEnabled());

      float initialWait_s = GetRNG().RandDblInRange(_minWaitTime_s, _maxWaitTime_s);
      
      float firstTurnDir = GetRNG().RandDbl() > 0.5f ? 1.0f : -1.0f;      
      float firstAngle_rads = firstTurnDir * GetRNG().RandDblInRange(_minSearchAngle_rads, _maxSearchAngle_rads);
      float afterFirstTurnWait_s = GetRNG().RandDblInRange(_minWaitTime_s, _maxWaitTime_s);

      float secondAngle_rads = -firstAngle_rads
        - firstTurnDir * GetRNG().RandDblInRange(_minSearchAngle_rads, _maxSearchAngle_rads);
      float afterSecondTurnWait_s = GetRNG().RandDblInRange(_minWaitTime_s, _maxWaitTime_s);

      PRINT_NAMED_DEBUG("SearchForNearbyObjectAction.Init",
                        "Action will wait %f, turn %fdeg, wait %f, turn %fdeg, wait %f",
                        initialWait_s,
                        RAD_TO_DEG(firstAngle_rads),
                        afterFirstTurnWait_s,
                        RAD_TO_DEG(secondAngle_rads),
                        afterSecondTurnWait_s);

      _compoundAction.AddAction(new WaitAction(_robot, initialWait_s));
      
      IActionRunner* driveAndLook = new CompoundActionParallel(_robot, {
        new DriveStraightAction(_robot, _backupDistance_mm, _backupSpeed_mms, false),
        new MoveHeadToAngleAction(_robot,_headAngle_rad)
      });
      
      
      _compoundAction.AddAction(driveAndLook);
      
      _compoundAction.AddAction(new WaitAction(_robot, initialWait_s));

      TurnInPlaceAction* turn0 = new TurnInPlaceAction(_robot, firstAngle_rads, false);
      turn0->SetTolerance(DEG_TO_RAD(4.0f));
      _compoundAction.AddAction(turn0);
      
      _compoundAction.AddAction(new WaitAction(_robot, afterFirstTurnWait_s));

      TurnInPlaceAction* turn1 = new TurnInPlaceAction(_robot, secondAngle_rads, false);
      turn1->SetTolerance(DEG_TO_RAD(4.0f));
      _compoundAction.AddAction(turn1);

      _compoundAction.AddAction(new WaitAction(_robot, afterSecondTurnWait_s));
      
      // Prevent the compound action from signaling completion
      _compoundAction.ShouldEmitCompletionSignal(false);
      
      // Prevent the compound action from locking tracks (the PanAndTiltAction handles it itself)
      _compoundAction.ShouldSuppressTrackLocking(true);

      // disable the live idle animation, so we aren't moving during the "wait" sections
      if( ! _shouldPopIdle ) {
        _shouldPopIdle = true;
        _robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
      }

      if(_desiredObjectID.IsSet())
      {
        const ObservableObject* desiredObject = _robot.GetBlockWorld().GetObjectByID(_desiredObjectID);
        if(nullptr != desiredObject)
        {
          _desiredObjectLastObsTime = desiredObject->GetLastObservedTime();
        }
      }
      
      // Go ahead and do the first Update for the compound action so we don't
      // "waste" the first CheckIfDone call doing so. Proceed so long as this
      // first update doesn't _fail_
      ActionResult compoundResult = _compoundAction.Update();
      if(ActionResult::SUCCESS == compoundResult ||
         ActionResult::RUNNING == compoundResult)
      {
        return ActionResult::SUCCESS;
      } else {
        return compoundResult;
      }
    }

    ActionResult SearchForNearbyObjectAction::CheckIfDone()
    {
      ActionResult internalResult = _compoundAction.Update();
      const ObservableObject* desiredObject = _desiredObjectID.IsSet() ? _robot.GetBlockWorld().GetObjectByID(_desiredObjectID) : nullptr;
      
      // check if the object has been located and actually observed
      if(desiredObject != nullptr &&
         desiredObject->IsPoseStateKnown() &&
         desiredObject->GetLastObservedTime() > _desiredObjectLastObsTime)
      {
        return ActionResult::SUCCESS;
      }
      
      // unsuccessful in finding the object
      else if(internalResult == ActionResult::SUCCESS && _desiredObjectID.IsSet()){
        return ActionResult::VISUAL_OBSERVATION_FAILED;
      }
      
      return internalResult;
    }
  
#pragma mark ---- DriveStraightAction ----
    
    DriveStraightAction::DriveStraightAction(Robot& robot, f32 dist_mm, f32 speed_mmps, bool shouldPlayAnimation)
    : IAction(robot,
              "DriveStraight",
              RobotActionType::DRIVE_STRAIGHT,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _dist_mm(dist_mm)
    , _speed_mmps(speed_mmps)
    , _shouldPlayDrivingAnimation(shouldPlayAnimation)
    {
      if(_speed_mmps < 0.f) {
        PRINT_NAMED_WARNING("DriveStraightAction.Constructor.NegativeSpeed",
                            "Speed should always be positive (not %f). Making positive.",
                            _speed_mmps);
        _speed_mmps = -_speed_mmps;
      }
      
      if(dist_mm < 0.f) {
        // If distance is negative, we are driving backward and will negate speed
        // internally. Yes, we could have just double-negated if the caller passed in
        // a negative speed already, but this avoids confusion on caller's side about
        // which signs to use and the documentation says speed should always be positive.
        DEV_ASSERT(_speed_mmps >= 0.f, "DriveStraightAction.Constructor.NegativeSpeed");
        _speed_mmps = -_speed_mmps;
      }
      
      SetName("DriveStraight" + std::to_string(_dist_mm) + "mm@" +
              std::to_string(_speed_mmps) + "mmps");
    }

    DriveStraightAction::~DriveStraightAction()
    {
      _robot.AbortDrivingToPose();
      _robot.GetContext()->GetVizManager()->ErasePath(_robot.GetID());

      _robot.GetDrivingAnimationHandler().ActionIsBeingDestroyed();
    }
  
    ActionResult DriveStraightAction::Init()
    {
      _robot.GetDrivingAnimationHandler().Init(GetTracksToLock(), GetTag(), IsSuppressingTrackLocking());
      
      if(_dist_mm == 0.f) {
        // special case
        _hasStarted = true;
        return ActionResult::SUCCESS;
      }
      
      const Radians heading = _robot.GetPose().GetRotation().GetAngleAroundZaxis();
      
      const Vec3f& T = _robot.GetDriveCenterPose().GetTranslation();
      const f32 x_start = T.x();
      const f32 y_start = T.y();
      
      const f32 x_end = x_start + _dist_mm * std::cos(heading.ToFloat());
      const f32 y_end = y_start + _dist_mm * std::sin(heading.ToFloat());
      
      Planning::Path path;
      // TODO: does matID matter? I'm just using 0 below
      if(false  == path.AppendLine(0, x_start, y_start, x_end, y_end,
                                   _speed_mmps, _accel_mmps2, _decel_mmps2))
      {
        PRINT_NAMED_ERROR("DriveStraightAction.Init.AppendLineFailed", "");
        return ActionResult::PATH_PLANNING_FAILED_ABORT;
      }
      
      _hasStarted = false;
      
      // Tell robot to execute this simple path
      if(RESULT_OK != _robot.ExecutePath(path, false)) {
        return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
      }
      
      return ActionResult::SUCCESS;
    }
    
    ActionResult DriveStraightAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;

      if(_robot.GetDrivingAnimationHandler().IsPlayingEndAnim())
      {
        return ActionResult::RUNNING;
      }
      else if ( _hasStarted && !_robot.IsTraversingPath() ) {
        result = ActionResult::SUCCESS;;
      }
      
      if(!_hasStarted) {
        PRINT_CH_INFO("Actions", "DriveStraightAction.CheckIfDone.WaitingForPathStart", "");
        _hasStarted = _robot.IsTraversingPath();
        if( _hasStarted && _shouldPlayDrivingAnimation) {
          _robot.GetDrivingAnimationHandler().PlayStartAnim();
        }
      } else if(/*hasStarted AND*/ !_robot.IsTraversingPath() && _shouldPlayDrivingAnimation) {
        if( _robot.GetDrivingAnimationHandler().PlayEndAnim()) {
          return ActionResult::RUNNING;
        }
        else {
          result = ActionResult::SUCCESS;
        }
      }
      
      return result;
    }
    
    
#pragma mark ---- CalibrateMotorAction ----
    
    CalibrateMotorAction::CalibrateMotorAction(Robot& robot,
                                               bool calibrateHead,
                                               bool calibrateLift)
    : IAction(robot,
              "CalibrateMotor-" + std::string(calibrateHead ? "Head" : "") + std::string(calibrateLift ? "Lift" : ""),
              RobotActionType::CALIBRATE_MOTORS,
              (calibrateHead ? (u8)AnimTrackFlag::HEAD_TRACK : 0) | (calibrateLift ? (u8)AnimTrackFlag::LIFT_TRACK : 0) )
    , _calibHead(calibrateHead)
    , _calibLift(calibrateLift)
    , _headCalibStarted(false)
    , _liftCalibStarted(false)
    {
      
    }
    
    ActionResult CalibrateMotorAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      _headCalibStarted = false;
      _liftCalibStarted = false;
      if (RESULT_OK != _robot.GetMoveComponent().CalibrateMotors(_calibHead, _calibLift)) {
        return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
      }
      
      if(_robot.HasExternalInterface())
      {
        using namespace ExternalInterface;
        auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
        helper.SubscribeEngineToGame<MessageEngineToGameTag::MotorCalibration>();
      }
      
      return result;
    }
    
    ActionResult CalibrateMotorAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      bool headCalibrating = !_robot.IsHeadCalibrated();
      bool liftCalibrating = !_robot.IsLiftCalibrated();
      
      bool headComplete = !_calibHead || (_headCalibStarted && !headCalibrating);
      bool liftComplete = !_calibLift || (_liftCalibStarted && !liftCalibrating);
      if (headComplete && liftComplete) {
        PRINT_NAMED_INFO("CalibrateMotorAction.CheckIfDone.Done", "");
        result = ActionResult::SUCCESS;
      }

      return result;
    }
    
    template<>
    void CalibrateMotorAction::HandleMessage(const MotorCalibration& msg)
    {
      if (msg.calibStarted) {
        if (msg.motorID == MotorID::MOTOR_HEAD) {
          _headCalibStarted = true;
        }
        if (msg.motorID == MotorID::MOTOR_LIFT) {
          _liftCalibStarted = true;
        }
      }
    }
    
#pragma mark ---- MoveHeadToAngleAction ----
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(Robot& robot, const Radians& headAngle, const Radians& tolerance, const Radians& variability)
    : IAction(robot,
              "MoveHeadTo" + std::to_string(headAngle.getDegrees()) + "Deg",
              RobotActionType::MOVE_HEAD_TO_ANGLE,
              (u8)AnimTrackFlag::HEAD_TRACK)
    , _headAngle(headAngle)
    , _angleTolerance(tolerance)
    , _variability(variability)
    , _inPosition(false)
    {
      if(_headAngle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor.AngleTooLow",
                            "Requested head angle (%.1fdeg) less than min head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MIN_HEAD_ANGLE));
        _headAngle = MIN_HEAD_ANGLE;
      } else if(_headAngle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor.AngleTooHigh",
                            "Requested head angle (%.1fdeg) more than max head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MAX_HEAD_ANGLE));
        _headAngle = MAX_HEAD_ANGLE;
      }
      
      if( _angleTolerance.ToFloat() < HEAD_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            _angleTolerance.getDegrees(),
                            RAD_TO_DEG(HEAD_ANGLE_TOL));
        _angleTolerance = HEAD_ANGLE_TOL;
      }
      
      if(_variability > 0) {
        _headAngle += GetRNG().RandDblInRange(-_variability.ToDouble(), _variability.ToDouble());
        _headAngle = CLIP(_headAngle, MIN_HEAD_ANGLE, MAX_HEAD_ANGLE);
      }
    }
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(Robot& robot, const Preset preset, const Radians& tolerance, const Radians& variability)
      : MoveHeadToAngleAction(robot, GetPresetHeadAngle(preset), tolerance, variability)
    {
      SetName(std::string("MoveHeadTo_") + GetPresetName(preset));
    }
    
    f32 MoveHeadToAngleAction::GetPresetHeadAngle(Preset preset)
    {
      switch(preset) {
        case Preset::GROUND_PLANE_VISIBLE: { return DEG_TO_RAD(-15.0f); }
      }
      DEV_ASSERT(false, "MoveHeadToAngleAction.NotAPreset");
      return -1.0f;
    }
    
    const char* MoveHeadToAngleAction::GetPresetName(Preset preset)
    {
      switch(preset) {
        case Preset::GROUND_PLANE_VISIBLE: { return "GroundPlaneVisible"; }
      }
      DEV_ASSERT(false, "MoveHeadToAngleAction.NotAPreset");
      return "ERROR";
    }
    
    MoveHeadToAngleAction::~MoveHeadToAngleAction()
    {
      if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag)
      {
        // Make sure eye shift gets removed, by this action, or by the MoveComponent if "hold" is enabled
        if(_holdEyes) {
          _robot.GetMoveComponent().RemoveFaceLayerWhenHeadMoves(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
        } else {
          _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
        }
        _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
      }
    }
    
    bool MoveHeadToAngleAction::IsHeadInPosition() const
    {
      const bool inPosition = _headAngle.IsNear(_robot.GetHeadAngle(), _angleTolerance.ToFloat()+Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
      return inPosition;
    }
    
    ActionResult MoveHeadToAngleAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      _inPosition = IsHeadInPosition();
      
      if(!_inPosition) {
        if(RESULT_OK != _robot.GetMoveComponent().MoveHeadToAngle(_headAngle.ToFloat(),
                                                                   _maxSpeed_radPerSec,
                                                                   _accel_radPerSec2,
                                                                   _duration_sec))
        {
          result = ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
        }
        
        if(_moveEyes)
        {
          // Remove any existing eye dart due to keeping face alive, since we're doing our own
          _robot.GetAnimationStreamer().RemoveKeepAliveEyeDart(IKeyFrame::SAMPLE_LENGTH_MS);
          
          // Lead with the eyes, if not in position
          // Note: assuming screen is about the same x distance from the neck joint as the head cam
          Radians angleDiff =  _robot.GetHeadAngle() - _headAngle;
          const f32 y_mm = std::tan(angleDiff.ToFloat()) * HEAD_CAM_POSITION[0];
          const f32 yPixShift = y_mm * (static_cast<f32>(ProceduralFace::HEIGHT/4) / SCREEN_SIZE[1]);
          
          _robot.ShiftEyes(_eyeShiftTag, 0, yPixShift, 4*IKeyFrame::SAMPLE_LENGTH_MS, "MoveHeadToAngleEyeShift");
          
          if(!_holdEyes) {
            // Store the half the angle differene so we know when to remove eye shift
            _halfAngle = 0.5f*(_headAngle - _robot.GetHeadAngle()).getAbsoluteVal();
          }
        }
      }
      
      return result;
    }
    
    ActionResult MoveHeadToAngleAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsHeadInPosition();
      }
      
      if(!_holdEyes && AnimationStreamer::NotAnimatingTag != _eyeShiftTag)
      {
        // If we're not there yet but at least halfway, and we're not supposed
        // to "hold" the eyes, then remove eye shift
        if(_inPosition || _headAngle.IsNear(_robot.GetHeadAngle(), _halfAngle))
        {
          PRINT_CH_DEBUG("Actions", "MoveHeadToAngleAction.CheckIfDone.RemovingEyeShift",
                         "[%d] Currently at %.1fdeg, on the way to %.1fdeg, within "
                         "half angle of %.1fdeg",
                         GetTag(),
                         RAD_TO_DEG(_robot.GetHeadAngle()),
                         _headAngle.getDegrees(),
                         _halfAngle.getDegrees());
          
          _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
      }
      
      if( _robot.GetMoveComponent().IsHeadMoving() ) {
        _motionStarted = true;
      }
      
      // Wait to get a state message back from the physical robot saying its head
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(_inPosition) {
      
        if(_robot.GetMoveComponent().IsHeadMoving())
        {
          PRINT_CH_INFO("Actions",
                        "MoveHeadToAngleAction.CheckIfDone.HeadMovingInPosition",
                        "[%d] Head considered in position at %.1fdeg but still moving at %.1fdeg",
                        GetTag(),
                        _headAngle.getDegrees(),
                        RAD_TO_DEG(_robot.GetHeadAngle()));
        }
      
        result = _robot.GetMoveComponent().IsHeadMoving() ? ActionResult::RUNNING : ActionResult::SUCCESS;
      } else {
        // Don't spam "not in position messages"
        PRINT_PERIODIC_CH_DEBUG(10, "Actions", "MoveHeadToAngleAction.CheckIfDone.NotInPosition",
                                "[%d] Waiting for head to get in position: %.1fdeg vs. %.1fdeg(+/-%.1f) tol:%.1fdeg",
                                GetTag(),
                                RAD_TO_DEG(_robot.GetHeadAngle()),
                                _headAngle.getDegrees(),
                                _variability.getDegrees(),
                                _angleTolerance.getDegrees());
        
        if( _motionStarted && ! _robot.GetMoveComponent().IsHeadMoving() ) {
          PRINT_NAMED_WARNING("MoveHeadToAngleAction.CheckIfDone.StoppedMakingProgress",
                              "[%d] giving up since we stopped moving",
                              GetTag());
          result = ActionResult::MOTOR_STOPPED_MAKING_PROGRESS;
        }
      }
      
      return result;
    }
    
#pragma mark ---- MoveLiftToHeightAction ----
    
    MoveLiftToHeightAction::MoveLiftToHeightAction(Robot& robot, const f32 height_mm, const f32 tolerance_mm, const f32 variability)
    : IAction(robot,
              "MoveLiftTo" + std::to_string(height_mm) + "mm",
              RobotActionType::MOVE_LIFT_TO_HEIGHT,
              (u8)AnimTrackFlag::LIFT_TRACK)
    , _height_mm(height_mm)
    , _heightTolerance(tolerance_mm)
    , _variability(variability)
    , _inPosition(false)
    {
      
    }
    
    MoveLiftToHeightAction::MoveLiftToHeightAction(Robot& robot, const Preset preset, const f32 tolerance_mm)
    : MoveLiftToHeightAction(robot, GetPresetHeight(preset), tolerance_mm, 0.f)
    {
      SetName("MoveLiftTo" + GetPresetName(preset));
    }
    
    
    f32 MoveLiftToHeightAction::GetPresetHeight(Preset preset)
    {
      static const std::map<Preset, f32> LUT = {
        {Preset::LOW_DOCK,   LIFT_HEIGHT_LOWDOCK},
        {Preset::HIGH_DOCK,  LIFT_HEIGHT_HIGHDOCK},
        {Preset::CARRY,      LIFT_HEIGHT_CARRY},
        {Preset::OUT_OF_FOV, -1.f},
      };
      
      return LUT.at(preset);
    }
    
    const std::string& MoveLiftToHeightAction::GetPresetName(Preset preset)
    {
      static const std::map<Preset, std::string> LUT = {
        {Preset::LOW_DOCK,   "LowDock"},
        {Preset::HIGH_DOCK,  "HighDock"},
        {Preset::CARRY,      "HeightCarry"},
        {Preset::OUT_OF_FOV, "OutOfFOV"},
      };
      
      static const std::string unknown("UnknownPreset");
      
      auto iter = LUT.find(preset);
      if(iter == LUT.end()) {
        return unknown;
      } else {
        return iter->second;
      }
    }
    
    bool MoveLiftToHeightAction::IsLiftInPosition() const
    {
      const bool inPosition = (NEAR(_heightWithVariation, _robot.GetLiftHeight(), _heightTolerance) &&
                               !_robot.GetMoveComponent().IsLiftMoving());
      
      return inPosition;
    }
    
    ActionResult MoveLiftToHeightAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if (_height_mm >= 0 && (_height_mm < LIFT_HEIGHT_LOWDOCK || _height_mm > LIFT_HEIGHT_CARRY)) {
        PRINT_NAMED_WARNING("MoveLiftToHeightAction.Init.InvalidHeight",
                            "%f mm. Clipping to be in range.", _height_mm);
        _height_mm = CLIP(_height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }
      
      if(_height_mm < 0.f) {
        // Choose whatever is closer to current height, LOW or CARRY:
        const f32 currentHeight = _robot.GetLiftHeight();
        const f32 low   = GetPresetHeight(Preset::LOW_DOCK);
        const f32 carry = GetPresetHeight(Preset::CARRY);
        // Absolute values here shouldn't be necessary, since these are supposed
        // to be the lowest and highest possible lift settings, but just in case...
        if( std::abs(currentHeight-low) < std::abs(carry-currentHeight)) {
          _heightWithVariation = low;
        } else {
          _heightWithVariation = carry;
        }
      } else {
        _heightWithVariation = _height_mm;
        if(_variability > 0.f) {
          _heightWithVariation += GetRNG().RandDblInRange(-_variability, _variability);
        }
        _heightWithVariation = CLIP(_heightWithVariation, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }
      
      
      // Convert height tolerance to angle tolerance and make sure that it's larger
      // than the tolerance that the liftController uses.
      
      // Convert target height, height - tol, and height + tol to angles.
      f32 heightLower = _heightWithVariation - _heightTolerance;
      f32 heightUpper = _heightWithVariation + _heightTolerance;
      f32 targetAngle = Robot::ConvertLiftHeightToLiftAngleRad(_heightWithVariation);
      f32 targetAngleLower = Robot::ConvertLiftHeightToLiftAngleRad(heightLower);
      f32 targetAngleUpper = Robot::ConvertLiftHeightToLiftAngleRad(heightUpper);
      
      // Neither of the angular differences between targetAngle and its associated
      // lower and upper tolerance limits should be smaller than LIFT_ANGLE_TOL.
      // That is, unless the limits exceed the physical limits of the lift.
      f32 minAngleDiff = std::numeric_limits<f32>::max();
      if (heightLower > LIFT_HEIGHT_LOWDOCK) {
        minAngleDiff = targetAngle - targetAngleLower;
      }
      if (heightUpper < LIFT_HEIGHT_CARRY) {
        minAngleDiff = std::min(minAngleDiff, targetAngleUpper - targetAngle);
      }
      
      if (minAngleDiff < LIFT_ANGLE_TOL) {
        // Tolerance is too small. Clip to be within range.
        f32 desiredHeightLower = Robot::ConvertLiftAngleToLiftHeightMM(targetAngle - LIFT_ANGLE_TOL);
        f32 desiredHeightUpper = Robot::ConvertLiftAngleToLiftHeightMM(targetAngle + LIFT_ANGLE_TOL);
        f32 newHeightTolerance = std::max(_height_mm - desiredHeightLower, desiredHeightUpper - _height_mm);
        
        PRINT_NAMED_WARNING("MoveLiftToHeightAction.Init.TolTooSmall",
                            "HeightTol %f mm == AngleTol %f rad near height of %f mm. Clipping tol to %f mm",
                            _heightTolerance, minAngleDiff, _heightWithVariation, newHeightTolerance);
        _heightTolerance = newHeightTolerance;
      }
      
      _inPosition = IsLiftInPosition();
      
      if(!_inPosition) {
        if(_robot.GetMoveComponent().MoveLiftToHeight(_heightWithVariation,
                                                       _maxLiftSpeedRadPerSec,
                                                       _liftAccelRacPerSec2,
                                                       _duration) != RESULT_OK) {
          result = ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
        }
      }
      
      return result;
    }
    
    ActionResult MoveLiftToHeightAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsLiftInPosition();
      }
      
      // TODO: Somehow verify robot got command to move lift before declaring success
      /*
       // Wait for the lift to start moving (meaning robot received command) and
       // then stop moving
       static bool liftStartedMoving = false;
       if(!liftStartedMoving) {
       liftStartedMoving = _robot.IsLiftMoving();
       }
       else
       */
      
      if( _robot.GetMoveComponent().IsLiftMoving() ) {
        _motionStarted = true;
      }
      
      if(_inPosition) {
        result = _robot.GetMoveComponent().IsLiftMoving() ? ActionResult::RUNNING : ActionResult::SUCCESS;
      } else {
        PRINT_PERIODIC_CH_DEBUG(10, "Actions", "MoveLiftToHeightAction.CheckIfDone.NotInPosition",
                                "[%d] Waiting for lift to get in position: %.1fmm vs. %.1fmm (tol: %f)",
                                GetTag(),
                                _robot.GetLiftHeight(), _heightWithVariation, _heightTolerance);
        
        if( _motionStarted && ! _robot.GetMoveComponent().IsLiftMoving() ) {
          PRINT_NAMED_WARNING("MoveLiftToHeightAction.CheckIfDone.StoppedMakingProgress",
                              "[%d] giving up since we stopped moving",
                              GetTag());
          result = ActionResult::MOTOR_STOPPED_MAKING_PROGRESS;
        }
      }
      
      return result;
    }
    
#pragma mark ---- PanAndTiltAction ----
    
    PanAndTiltAction::PanAndTiltAction(Robot& robot, Radians bodyPan, Radians headTilt,
                                       bool isPanAbsolute, bool isTiltAbsolute)
    : IAction(robot,
              "PanAndTilt",
              RobotActionType::PAN_AND_TILT,
              (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK)
    , _compoundAction(robot)
    , _bodyPanAngle(bodyPan)
    , _headTiltAngle(headTilt)
    , _isPanAbsolute(isPanAbsolute)
    , _isTiltAbsolute(isTiltAbsolute)
    {
      // Put the angles in the name for debugging
      SetName("Pan" + std::to_string(std::round(_bodyPanAngle.getDegrees())) +
              "AndTilt" + std::to_string(std::round(_headTiltAngle.getDegrees())));
    }
    
    PanAndTiltAction::~PanAndTiltAction()
    {
      _compoundAction.PrepForCompletion();
    }
    
    void PanAndTiltAction::SetMaxPanSpeed(f32 maxSpeed_radPerSec)
    {
      if (maxSpeed_radPerSec == 0.f) {
        _maxPanSpeed_radPerSec = _kDefaultMaxPanSpeed;
      } else if (std::fabsf(maxSpeed_radPerSec) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        PRINT_NAMED_WARNING("PanAndTiltAction.SetMaxSpeed.PanSpeedExceedsLimit",
                            "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
                            RAD_TO_DEG(maxSpeed_radPerSec), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _maxPanSpeed_radPerSec = std::copysign(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, maxSpeed_radPerSec);
      } else {
        _maxPanSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void PanAndTiltAction::SetPanAccel(f32 accel_radPerSec2)
    {
      // If 0, use default value
      if (accel_radPerSec2 == 0.f) {
        _panAccel_radPerSec2 = _kDefaultPanAccel;
      } else {
        _panAccel_radPerSec2 = accel_radPerSec2;
      }
    }
    
    void PanAndTiltAction::SetPanTolerance(const Radians& angleTol_rad)
    {
      if (angleTol_rad == 0.f) {
        _panAngleTol = _kDefaultPanAngleTol;
        return;
      }
      
      _panAngleTol = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _panAngleTol.ToFloat() < POINT_TURN_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("PanAndTiltAction.SetPanTolerance.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            _panAngleTol.getDegrees(),
                            RAD_TO_DEG(POINT_TURN_ANGLE_TOL));
        _panAngleTol = POINT_TURN_ANGLE_TOL;
      }
    }
    
    void PanAndTiltAction::SetMaxTiltSpeed(f32 maxSpeed_radPerSec)
    {
      if (maxSpeed_radPerSec == 0.f) {
        _maxTiltSpeed_radPerSec = _kDefaultMaxTiltSpeed;
      } else {
        _maxTiltSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void PanAndTiltAction::SetTiltAccel(f32 accel_radPerSec2)
    {
      if (accel_radPerSec2 == 0.f) {
        _tiltAccel_radPerSec2 = _kDefaultTiltAccel;
      } else {
        _tiltAccel_radPerSec2 = accel_radPerSec2;
      }
    }
    
    void PanAndTiltAction::SetTiltTolerance(const Radians& angleTol_rad)
    {
      // If 0, use default value
      if (angleTol_rad == 0.f) {
        _tiltAngleTol = _kDefaultTiltAngleTol;
        return;
      }
      
      _tiltAngleTol = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _tiltAngleTol.ToFloat() < HEAD_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("PanAndTiltAction.SetTiltTolerance.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            _tiltAngleTol.getDegrees(),
                            RAD_TO_DEG(HEAD_ANGLE_TOL));
        _tiltAngleTol = HEAD_ANGLE_TOL;
      }
    }
    
    ActionResult PanAndTiltAction::Init()
    {
      // Incase we are re-running this action
      _compoundAction.ClearActions();
      _compoundAction.EnableMessageDisplay(IsMessageDisplayEnabled());
      
      TurnInPlaceAction* action = new TurnInPlaceAction(_robot, _bodyPanAngle, _isPanAbsolute);
      action->SetTolerance(_panAngleTol);
      action->SetMaxSpeed(_maxPanSpeed_radPerSec);
      action->SetAccel(_panAccel_radPerSec2);
      action->SetMoveEyes(_moveEyes);
      _compoundAction.AddAction(action);
      
      const Radians newHeadAngle = _isTiltAbsolute ? _headTiltAngle : _robot.GetHeadAngle() + _headTiltAngle;
      MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction(_robot, newHeadAngle, _tiltAngleTol);
      headAction->SetMaxSpeed(_maxTiltSpeed_radPerSec);
      headAction->SetAccel(_tiltAccel_radPerSec2);
      headAction->SetMoveEyes(_moveEyes);
      _compoundAction.AddAction(headAction);
      
      // Prevent the compound action from signaling completion
      _compoundAction.ShouldEmitCompletionSignal(false);
      
      // Prevent the compound action from locking tracks (the PanAndTiltAction handles it itself)
      _compoundAction.ShouldSuppressTrackLocking(true);
      
      // Go ahead and do the first Update for the compound action so we don't
      // "waste" the first CheckIfDone call doing so. Proceed so long as this
      // first update doesn't _fail_
      ActionResult compoundResult = _compoundAction.Update();
      if(ActionResult::SUCCESS == compoundResult ||
         ActionResult::RUNNING == compoundResult)
      {
        return ActionResult::SUCCESS;
      } else {
        return compoundResult;
      }
      
    } // PanAndTiltAction::Init()
    
    
    ActionResult PanAndTiltAction::CheckIfDone()
    {
      return _compoundAction.Update();
    }
    #pragma mark ---- TurnTowardsObjectAction ----
    
    TurnTowardsObjectAction::TurnTowardsObjectAction(Robot& robot,
                                       ObjectID objectID,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : TurnTowardsObjectAction(robot,
                              objectID,
                              Vision::Marker::ANY_CODE,
                              maxTurnAngle,
                              visuallyVerifyWhenDone,
                              headTrackWhenDone)
    {

    }
    
    TurnTowardsObjectAction::TurnTowardsObjectAction(Robot& robot,
                                       ObjectID objectID,
                                       Vision::Marker::Code whichCode,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : TurnTowardsPoseAction(robot, maxTurnAngle)
    , _facePoseCompoundActionDone(false)
    , _visuallyVerifyWhenDone(visuallyVerifyWhenDone)
    , _objectID(objectID)
    , _whichCode(whichCode)
    , _headTrackWhenDone(headTrackWhenDone)
    {
      SetName("TurnTowardsObject" + std::to_string(_objectID.GetValue()));
      SetType(RobotActionType::TURN_TOWARDS_OBJECT);
    }
    
    TurnTowardsObjectAction::~TurnTowardsObjectAction()
    {
      if(nullptr != _visuallyVerifyAction) {
        _visuallyVerifyAction->PrepForCompletion();
        Util::SafeDelete(_visuallyVerifyAction);
      }
    }

    void TurnTowardsObjectAction::UseCustomObject(ObservableObject* objectPtr)
    {
      if( _objectID.IsSet() ) {
        PRINT_NAMED_WARNING("TurnTowardsObjectAction.UseCustomObject.CustomObjectOverwriteId",
                            "object id was already set to %d, but now setting it to use a custom object ptr",
                            _objectID.GetValue());
        _objectID.UnSet();
      }
      
      _objectPtr = objectPtr;
      
      SetName("TurnTowardsCustomObject" + std::to_string(_objectPtr->GetID().GetValue()));
      
      if(!_objectPtr->GetID().IsSet())
      {
        PRINT_CH_INFO("Actions", "TurnTowardsObjectAction.UseCustomObject.NoCustomID", "");
      }
    }

    ActionResult TurnTowardsObjectAction::Init()
    {
      const bool usingCustomObject = !_objectID.IsSet();
      
      if(usingCustomObject)
      {
        if(nullptr == _objectPtr)
        {
          PRINT_NAMED_ERROR("TurnTowardsObjectAction.Init.NullCustomObject", "");
          return ActionResult::BAD_OBJECT;
        }
        
        // A custom object's pose must be in the robot's origin to turn towards it
        const Pose3d* objectOrigin = &_objectPtr->GetPose().FindOrigin();
        const Pose3d* robotOrigin  = _robot.GetWorldOrigin();
        if(objectOrigin != robotOrigin) {
          PRINT_NAMED_WARNING("TurnTowardsObjectAction.Init.CustomObjectNotInRobotFrame",
                              "Custom %s object %d in origin:%s vs. robot in origin:%s",
                              EnumToString(_objectPtr->GetType()),
                              _objectPtr->GetID().GetValue(),
                              objectOrigin->GetName().c_str(),
                              robotOrigin->GetName().c_str());
          return ActionResult::BAD_POSE;
        }
        
        if(_visuallyVerifyWhenDone)
        {
          PRINT_NAMED_WARNING("TurnTowardsObjectAction.Init.CannotVisuallyVerifyCustomObject",
                              "Disabling visual verification");
          _visuallyVerifyWhenDone = false;
        }
      }
      else
      {
        _objectPtr = _robot.GetBlockWorld().GetObjectByID(_objectID);
        if(nullptr == _objectPtr) {
          PRINT_NAMED_WARNING("TurnTowardsObjectAction.Init.ObjectNotFound",
                              "Object with ID=%d no longer exists in the world.",
                              _objectID.GetValue());
          return ActionResult::BAD_OBJECT;
        }
      }
      
      Pose3d objectPoseWrtRobot;
      if(_whichCode == Vision::Marker::ANY_CODE) {

        // if ANY_CODE is specified, find the closest marker face to the robot and use that pose. We don't
        // want to consider the "top" or "bottom" faces (based on current rotation)

        // Solution: project all points into 2D and pick the closest. The top and bottom faces will never be
        // closer than the closest side face (unless we are inside the cube)
        
        const Result poseResult = _objectPtr->GetClosestMarkerPose(_robot.GetPose(), true, objectPoseWrtRobot);
        
        if( RESULT_OK != poseResult ) {
          // This should not occur because we checked above that the object is in the
          // same coordinate frame as the robot
          PRINT_NAMED_ERROR("TurnTowardsObjectAction.Init.NoValidPose",
                            "Could not get a valid closest marker pose of %sobject %d",
                            usingCustomObject ? "custom " : "",
                            _objectPtr->GetID().GetValue());
          return ActionResult::BAD_MARKER;
        }
      } else {
        // Use the closest marker with the specified code:
        std::vector<Vision::KnownMarker*> const& markers = _objectPtr->GetMarkersWithCode(_whichCode);
        
        if(markers.empty()) {
          PRINT_NAMED_ERROR("TurnTowardsObjectAction.Init.NoMarkersWithCode",
                            "%sbject %d does not have any markers with code %d.",
                            usingCustomObject ? "Custom o" : "O",
                            _objectPtr->GetID().GetValue(), _whichCode);
          return ActionResult::BAD_MARKER;
        }
        
        Vision::KnownMarker* closestMarker = nullptr;
        
        f32 closestDist = std::numeric_limits<f32>::max();
        Pose3d markerPoseWrtRobot;
        for(auto marker : markers) {
          if(false == marker->GetPose().GetWithRespectTo(_robot.GetPose(), markerPoseWrtRobot)) {
            PRINT_NAMED_ERROR("TurnTowardsObjectAction.Init.MarkerOriginProblem",
                              "Could not get pose of marker with code %d of %sobject %d "
                              "w.r.t. robot pose.", _whichCode,
                              usingCustomObject ? "custom " : "",
                              _objectPtr->GetID().GetValue());
            return ActionResult::BAD_POSE;
          }
          
          const f32 currentDist = markerPoseWrtRobot.GetTranslation().Length();
          if(currentDist < closestDist) {
            closestDist = currentDist;
            closestMarker = marker;
            objectPoseWrtRobot = markerPoseWrtRobot;
          }
        }
        
        if(closestMarker == nullptr) {
          PRINT_NAMED_ERROR("TurnTowardsObjectAction.Init.NoClosestMarker",
                            "No closest marker found for %sobject %d.",
                            usingCustomObject ? "custom " : "",
                            _objectPtr->GetID().GetValue());
          return ActionResult::BAD_MARKER;
        }
      }
      
      // Have to set the parent class's pose before calling its Init()
      SetPose(objectPoseWrtRobot);
      
      ActionResult facePoseInitResult = TurnTowardsPoseAction::Init();
      if(ActionResult::SUCCESS != facePoseInitResult) {
        return facePoseInitResult;
      }
      
      _facePoseCompoundActionDone = false;
      
      return ActionResult::SUCCESS;
    } // TurnTowardsObjectAction::Init()
    
    
    ActionResult TurnTowardsObjectAction::CheckIfDone()
    {
      // Tick the compound action until it completes
      if(!_facePoseCompoundActionDone) {
        ActionResult compoundResult = TurnTowardsPoseAction::CheckIfDone();
        
        if(compoundResult != ActionResult::SUCCESS) {
          return compoundResult;
        } else {
          _facePoseCompoundActionDone = true;
          
          if(_doRefinedTurn)
          {
            // If we need to refine the turn just reset this action, set appropriate variables
            Reset(false);
            ShouldDoRefinedTurn(false);
            SetMaxPanSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
            SetPanTolerance(_refinedTurnAngleTol_rad);
            
            PRINT_CH_INFO("Actions", "TurnTowardsObjectAction.CheckIfDone.RefiningTurn",
                          "Refining turn towards %sobject %d",
                          _objectID.IsSet() ? "" : "custom ",
                          _objectPtr->GetID().GetValue());
            
            return ActionResult::RUNNING;
          }
          else if(_visuallyVerifyWhenDone)
          {
            _visuallyVerifyAction = new VisuallyVerifyObjectAction(_robot, _objectPtr->GetID(), _whichCode);
            
            // Disable completion signals since this is inside another action
            _visuallyVerifyAction->ShouldEmitCompletionSignal(false);
            _visuallyVerifyAction->ShouldSuppressTrackLocking(true);
            
            // Go ahead and do a first tick of visual verification's Update, to
            // get it initialized
            ActionResult verificationResult = _visuallyVerifyAction->Update();
            if(ActionResult::SUCCESS != verificationResult) {
              return verificationResult;
            }
          }
        }
      }

      // If we get here, _compoundAction completed returned SUCCESS. So we can
      // can continue with our additional checks:
      if (nullptr != _visuallyVerifyAction) {
        ActionResult verificationResult = _visuallyVerifyAction->Update();
        if (verificationResult != ActionResult::SUCCESS) {
          return verificationResult;
        }
      }
      
      if(_headTrackWhenDone) {
        if( !_objectID.IsSet() ) {
          PRINT_NAMED_WARNING("TurnTowardsObjectAction.CustomObject.TrackingNotsupported",
                              "No valid object id (you probably specified a custom action), so can't track");
          // TODO:(bn) hang action here for consistency?
        }
        else {
          _robot.GetActionList().QueueAction(QueueActionPosition::NEXT,
                                             new TrackObjectAction(_robot, _objectID));
        }
      }
      return ActionResult::SUCCESS;
    } // TurnTowardsObjectAction::CheckIfDone()
    
    void TurnTowardsObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs[0] = _objectID;
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
    }
    
#pragma mark ---- TraverseObjectAction ----
    
    TraverseObjectAction::TraverseObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IActionRunner(robot,
                    "TraverseObject",
                    RobotActionType::TRAVERSE_OBJECT,
                    (u8)AnimTrackFlag::BODY_TRACK)
    , _objectID(objectID)
    , _useManualSpeed(useManualSpeed)
    {
  
    }
    
    void TraverseObjectAction::SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2) {
      _speed_mmps = speed_mmps;
      _accel_mmps2 = accel_mmps2;
    }
    
    ActionResult TraverseObjectAction::UpdateInternal()
    {
      // Select the chosen action based on the object's type, if we haven't
      // already
      if(_chosenAction == nullptr) {
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("TraverseObjectAction.UpdateInternal.ObjectNotFound",
                            "Could not get actionable object with ID = %d from world.", _objectID.GetValue());
          return ActionResult::BAD_OBJECT;
        }
        
        if(object->GetType() == ObjectType::Bridge_LONG ||
           object->GetType() == ObjectType::Bridge_SHORT)
        {
          CrossBridgeAction* bridgeAction = new CrossBridgeAction(_robot, _objectID, _useManualSpeed);
          bridgeAction->SetSpeedAndAccel(_speed_mmps, _accel_mmps2, _decel_mmps2);
          bridgeAction->ShouldSuppressTrackLocking(true);
          _chosenAction = bridgeAction;
        }
        else if(object->GetType() == ObjectType::Ramp_Basic) {
          AscendOrDescendRampAction* rampAction = new AscendOrDescendRampAction(_robot, _objectID, _useManualSpeed);
          rampAction->SetSpeedAndAccel(_speed_mmps, _accel_mmps2, _decel_mmps2);
          rampAction->ShouldSuppressTrackLocking(true);
          _chosenAction = rampAction;
        }
        else {
          PRINT_NAMED_ERROR("TraverseObjectAction.UpdateInternal.CannotTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that traversal is not defined.", _robot.GetID(),
                            object->GetID().GetValue(), ObjectTypeToString(object->GetType()));
          
          return ActionResult::BAD_OBJECT;
        }
      }
      
      // Now just use chosenAction's Update()
      assert(_chosenAction != nullptr);
      return _chosenAction->Update();
      
    } // Update()
    
#pragma mark ---- TurnTowardsPoseAction ----
    
    TurnTowardsPoseAction::TurnTowardsPoseAction(Robot& robot, const Pose3d& pose, Radians maxTurnAngle)
    : PanAndTiltAction(robot, 0, 0, false, true)
    , _poseWrtRobot(pose)
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    , _isPoseSet(true)
    {
      SetName("TurnTowardsPose");
      SetType(RobotActionType::TURN_TOWARDS_POSE);
    }
    
    TurnTowardsPoseAction::TurnTowardsPoseAction(Robot& robot, Radians maxTurnAngle)
    : PanAndTiltAction(robot, 0, 0, false, true)
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    , _isPoseSet(false)
    {
      
    }
    
    
    // Compute the required head angle to face the object
    // NOTE: It would be more accurate to take head tilt into account, but I'm
    //  just using neck joint height as an approximation for the camera's
    //  current height, since its actual height changes slightly as the head
    //  rotates around the neck.
    //  Also, the equation for computing the actual angle in closed form gets
    //  surprisingly nasty very quickly.
    Radians TurnTowardsPoseAction::GetAbsoluteHeadAngleToLookAtPose(const Point3f& translationWrtRobot)
    {
      const f32 heightDiff = translationWrtRobot.z() - NECK_JOINT_POSITION[2];
      const f32 distanceXY = Point2f(translationWrtRobot).Length() - NECK_JOINT_POSITION[0];
      
      // Adding bias to account for the fact that the camera tends to look lower than
      // desired on account of it being lower wrt neck joint.
      // Ramp bias down to 0 for distanceXY values from 150mm to 300mm.
      const f32 kFullBiasDist_mm = 150;
      const f32 kNoBiasDist_mm = 300;
      const f32 biasScaleFactorDist = CLIP((kNoBiasDist_mm - distanceXY) / (kNoBiasDist_mm - kFullBiasDist_mm), 0, 1);
      
      // Adding bias to account for the fact that we don't look high enough when turning towards objects off the ground
      // Apply full bias for object 10mm above neck joint and 0 for objects below neck joint
      const f32 kFullBiasHeight_mm = 10;
      const f32 kNoBiasHeight_mm = 0;
      const f32 biasScaleFactorHeight = CLIP((kNoBiasHeight_mm - heightDiff) / (kNoBiasHeight_mm - kFullBiasHeight_mm), 0, 1);
      
      // Adds 4 degrees to account for 4 degree lookdown on EP3
      const Radians headAngle = std::atan2(heightDiff, distanceXY) +
        (kHeadAngleDistBias_rad * biasScaleFactorDist) +
        (kHeadAngleHeightBias_rad * biasScaleFactorHeight) +
        DEG_TO_RAD(4);

      return headAngle;
    }

    Radians TurnTowardsPoseAction::GetRelativeBodyAngleToLookAtPose(const Point3f& translationWrtRobot)
    {
      return std::atan2(translationWrtRobot.y(),
                        translationWrtRobot.x());
    }
    
    void TurnTowardsPoseAction::SetPose(const Pose3d& pose)
    {
      _poseWrtRobot = pose;
      _isPoseSet = true;
    }
    
    ActionResult TurnTowardsPoseAction::Init()
    {
      // in case of re-run
      _nothingToDo = false;
      SetBodyPanAngle(0);
      
      if(!_isPoseSet) {
        PRINT_NAMED_ERROR("TurnTowardsPoseAction.Init.PoseNotSet", "");
        return ActionResult::BAD_POSE;
      }
      
      if(_poseWrtRobot.GetParent() == nullptr) {
        PRINT_CH_INFO("Actions", "TurnTowardsPoseAction.Init.AssumingRobotOriginAsParent", "");
        _poseWrtRobot.SetParent(_robot.GetWorldOrigin());
      }
      else if(false == _poseWrtRobot.GetWithRespectTo(_robot.GetPose(), _poseWrtRobot))
      {
        // TODO: It's possible this is just "normal" when dealing with delocalization, so possibly downgradable to Info later
        PRINT_NAMED_WARNING("TurnTowardsPoseAction.Init.PoseOriginFailure",
                            "Could not get pose (in frame %d) w.r.t. robot pose (in frame %d).",
                            _robot.GetPoseOriginList().GetOriginID(&_poseWrtRobot.FindOrigin()),
                            _robot.GetPoseOriginList().GetOriginID(_robot.GetWorldOrigin()));
        
        _poseWrtRobot.Print();
        _poseWrtRobot.PrintNamedPathToOrigin(false);
        _robot.GetPose().PrintNamedPathToOrigin(false);
        return ActionResult::BAD_POSE;
      }
      
      if(_maxTurnAngle > 0)
      {
        // Compute the required angle to face the object
        const Radians turnAngle = GetRelativeBodyAngleToLookAtPose(_poseWrtRobot.GetTranslation());
        
        PRINT_CH_INFO("Actions", "TurnTowardsPoseAction.Init.TurnAngle",
                         "Computed turn angle = %.1fdeg", turnAngle.getDegrees());
        
        if(turnAngle.getAbsoluteVal() <= _maxTurnAngle) {
          SetBodyPanAngle(turnAngle);
        } else {
          PRINT_CH_INFO("Actions", "TurnTowardsPoseAction.Init.RequiredTurnTooLarge",
                           "Required turn angle of %.1fdeg is larger than max angle of %.1fdeg.",
                            turnAngle.getDegrees(), _maxTurnAngle.getDegrees());
          
          _nothingToDo = true;
          return ActionResult::SUCCESS;
        }
      }
      
      // Compute the required head angle to face the object
      Radians headAngle;
      const f32 kYTolFrac = 0.01f; // Fraction of image height
      Result result = _robot.ComputeHeadAngleToSeePose(_poseWrtRobot, headAngle, kYTolFrac);
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("TurnTowardsPoseAction.Init.FailedToComputedHeadAngle",
                            "PoseWrtRobot translation=(%f,%f,%f)",
                            _poseWrtRobot.GetTranslation().x(),
                            _poseWrtRobot.GetTranslation().y(),
                            _poseWrtRobot.GetTranslation().z());
        
        // Fall back on old approximate method to compute head angle
        headAngle = GetAbsoluteHeadAngleToLookAtPose(_poseWrtRobot.GetTranslation());
      }
      
      headAngle = CLIP(headAngle, MIN_HEAD_ANGLE, MAX_HEAD_ANGLE);
      
      SetHeadTiltAngle(headAngle);
      
      // Proceed with base class's Init()
      return PanAndTiltAction::Init();
      
    } // TurnTowardsPoseAction::Init()
    
    ActionResult TurnTowardsPoseAction::CheckIfDone()
    {
      if(_nothingToDo) {
        return ActionResult::SUCCESS;
      } else {
        return PanAndTiltAction::CheckIfDone();
      }
    }
  
#pragma mark ---- TurnTowardsImagePointAction ----
    
    TurnTowardsImagePointAction::TurnTowardsImagePointAction(Robot& robot, const Point2f& imgPoint, const TimeStamp_t t)
    : PanAndTiltAction(robot, 0, 0, true, true)
    , _imgPoint(imgPoint)
    , _timestamp(t)
    {
      SetName("TurnTowardsImagePointAction");
      SetType(RobotActionType::TURN_TOWARDS_IMAGE_POINT);
    }
    
    ActionResult TurnTowardsImagePointAction::Init()
    {
      Radians panAngle, tiltAngle;
      Result result = _robot.ComputeTurnTowardsImagePointAngles(_imgPoint, _timestamp, panAngle, tiltAngle);
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("TurnTowardsImagePointAction.Init.ComputeTurnTowardsImagePointAnglesFailed",
                            "(%f,%f) at t=%u", _imgPoint.x(), _imgPoint.y(), _timestamp);
        return ActionResult::ABORT;
      }
      
      SetBodyPanAngle(panAngle);
      SetHeadTiltAngle(tiltAngle);
      
      return PanAndTiltAction::Init();
    }
    
#pragma mark ---- TurnTowardsLastFacePoseAction ----

    TurnTowardsFaceAction::TurnTowardsFaceAction(Robot& robot, Vision::FaceID_t faceID, Radians maxTurnAngle, bool sayName)
    : TurnTowardsPoseAction(robot, maxTurnAngle)
    , _faceID(faceID)
    , _sayName(sayName)
    {
      SetName("TurnTowardsFace" + std::to_string(faceID));
      SetType(RobotActionType::TURN_TOWARDS_FACE);
      SetTracksToLock((u8)AnimTrackFlag::NO_TRACKS);
    }

    void TurnTowardsFaceAction::SetAction(IActionRunner *action)
    {
      if(nullptr != _action) {
        _action->PrepForCompletion();
        Util::SafeDelete(_action);
      }
      _action = action;
      if(nullptr != _action) {
        _action->ShouldEmitCompletionSignal(false);
        _action->ShouldSuppressTrackLocking(true);
      }
    }
    
    TurnTowardsFaceAction::~TurnTowardsFaceAction()
    {
      SetAction(nullptr);
      
      // In case we got interrupted and didn't get a chance to do this
      if(_tracksLocked) {
        _robot.GetMoveComponent().UnlockTracks((u8)AnimTrackFlag::HEAD_TRACK |
                                               (u8)AnimTrackFlag::BODY_TRACK,
                                               GetTag());
      }
    }

    void TurnTowardsFaceAction::SetSayNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( ! _sayName ) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.SetSayNameTriggerWithoutSayingName",
                          "setting say name trigger, but we aren't going to say the name. This is useless");
      }
      _sayNameTriggerCallback = [trigger](const Robot& robot, Vision::FaceID_t faceID) {
        return trigger;
      };
    }

  void TurnTowardsFaceAction::SetNoNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( ! _sayName ) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.SetNoNameTriggerWithoutSayingName",
                          "setting anim trigger for unnamed faces, but we aren't going to say the name.");
      }
      _noNameTriggerCallback = [trigger](const Robot& robot, Vision::FaceID_t faceID) {
        return trigger;
      };
    }

    void TurnTowardsFaceAction::SetSayNameTriggerCallback(AnimTriggerForFaceCallback callback)
    {
      if( ! _sayName ) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.SetSayNameTriggerCallbackWithoutSayingName",
                          "setting say name trigger callback, but we aren't going to say the name. This is useless");
      }
      _sayNameTriggerCallback = callback;
    }      

    void TurnTowardsFaceAction::SetNoNameTriggerCallback(AnimTriggerForFaceCallback callback)
    {
      if( ! _sayName ) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.SetNoNameTriggerCallbackWithoutSayingName",
                          "setting say name trigger callback, but we aren't going to say the name. This is useless");
      }
      _noNameTriggerCallback = callback;
    }      

    ActionResult TurnTowardsFaceAction::Init()
    {
      // If we have a last observed face set, use its pose. Otherwise pose wil not be set
      // so fail if we require a face, skip ahead if we don't
      Pose3d pose;
      bool gotPose = false;

      
      if(_faceID != Vision::UnknownFaceID)
      {
        auto face = _robot.GetFaceWorld().GetFace(_faceID);
        if(nullptr != face && face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), pose)) {
          gotPose = true;
        }
      }
      else if(_robot.GetFaceWorld().GetLastObservedFaceWithRespectToRobot(pose) != 0)
      {
        gotPose = true;
      }
      
      if(gotPose)
      {
        TurnTowardsPoseAction::SetPose(pose);
        
        _action = nullptr;
        _obsFaceID = Vision::UnknownFaceID;
        _closestDistSq = std::numeric_limits<f32>::max();
        
        if(_robot.HasExternalInterface())
        {
          using namespace ExternalInterface;
          auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
          helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedFace>();
          helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotChangedObservedFaceID>();
        }
        
        _state = State::Turning;
        _robot.GetMoveComponent().LockTracks((u8)AnimTrackFlag::HEAD_TRACK |
                                             (u8)AnimTrackFlag::BODY_TRACK,
                                             GetTag(),
                                             GetName());
        _tracksLocked = true;
        
        return TurnTowardsPoseAction::Init();
      }
      else
      {
        if( _requireFaceConfirmation ) {
          PRINT_CH_INFO("Actions", "TurnTowardsFaceAction.Init.NoFacePose",
                        "Required face pose, don't have one, failing");
          return ActionResult::NO_FACE;
        }
        else {
          _state = State::SayingName; // jump to end and play animation (if present)
          return ActionResult::SUCCESS;
        }
      }
        
    } // Init()

    template<>
    void TurnTowardsFaceAction::HandleMessage(const ExternalInterface::RobotObservedFace& msg)
    {
      if(_state == State::Turning || _state == State::WaitingForFace)
      {
        Vision::FaceID_t faceID = msg.faceID;
        if(_faceID != Vision::UnknownFaceID)
        {
          // We know what face we're looking for. If this is it, set the observed face ID to it.
          if(faceID == _faceID) {
            _obsFaceID = faceID;
          }
        }
        else
        {
          // We are looking for any face.
          // Record this face if it is closer than any we've seen so far
          const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(faceID);
          if(nullptr != face) {
            Pose3d faceWrtRobot;
            if(true == face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), faceWrtRobot)) {
              const f32 distSq = faceWrtRobot.GetTranslation().LengthSq();
              if(distSq < _closestDistSq) {
                _obsFaceID = faceID;
                _closestDistSq = distSq;
                PRINT_NAMED_DEBUG("TurnTowardsFaceAction.ObservedFaceCallback",
                                  "Observed ID=%d at distSq=%.1f",
                                  _obsFaceID, _closestDistSq);
              }
            }
          }
        }
      }
    } // HandleMessage(RobotObservedFace)
    
    
    template<>
    void TurnTowardsFaceAction::HandleMessage(const ExternalInterface::RobotChangedObservedFaceID& msg)
    {
      if(_obsFaceID == msg.oldID) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.HandleChangedFaceIDMessage",
                          "Updating fine-tune observed ID from %d to %d",
                          _obsFaceID, msg.newID);
        _obsFaceID = msg.newID;
      }
      
      if(_faceID == msg.oldID) {
        PRINT_NAMED_DEBUG("TurnTowardsFaceAction.HandleChangedFaceIDMessage",
                          "Updating face ID from %d to %d",
                          _faceID, msg.newID);
        _faceID = msg.newID;
      }
    } // HandleMessage(RobotChangedObservedFaceID)
    
    
    void TurnTowardsFaceAction::CreateFineTuneAction()
    {
      PRINT_NAMED_DEBUG("TurnTowardsFaceAction.CreateFinalAction.SawFace",
                        "Observed ID=%d. Will fine tune.", _obsFaceID);
                        
      const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_obsFaceID);
      if(nullptr != face) {
        // Valid face...        
        Pose3d pose;
        if(true == face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), pose)) {
          _robot.GetMoodManager().TriggerEmotionEvent("LookAtFaceVerified", MoodManager::GetCurrentTimeInSeconds());
          
          // ... with valid pose w.r.t. robot. Turn towards that face -- iff it doesn't
          // require too large of an adjustment.
          const Radians maxFineTuneAngle( std::min( GetMaxTurnAngle().ToFloat(), DEG_TO_RAD(45.f)) );
          SetAction(new TurnTowardsPoseAction(_robot, pose, maxFineTuneAngle));
        }
      } else {
        SetAction(nullptr);
      }
      
      _state = State::FineTuning;
    } // CreateFineTuneAction()
    
    
    ActionResult TurnTowardsFaceAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      switch(_state)
      {
        case State::Turning:
        {
          result = TurnTowardsPoseAction::CheckIfDone();
          if(ActionResult::RUNNING != result) {
            _robot.GetMoveComponent().UnlockTracks((u8)AnimTrackFlag::HEAD_TRACK |
                                                   (u8)AnimTrackFlag::BODY_TRACK,
                                                   GetTag());
            _tracksLocked = false;
          }
          
          if(ActionResult::SUCCESS == result)
          {
            // Initial (blind) turning to pose finished...
            if(_obsFaceID == Vision::UnknownFaceID) {
              // ...didn't see a face yet, wait a couple of images to see if we do
              PRINT_NAMED_DEBUG("TurnTowardsFaceAction.CheckIfDone.NoFaceObservedYet",
                                "Will wait no more than %d frames",
                                _maxFramesToWait);
              DEV_ASSERT(nullptr == _action, "TurnTowardsFaceAction.CheckIfDone.ActionPointerShouldStillBeNull");
              SetAction(new WaitForImagesAction(_robot, _maxFramesToWait, VisionMode::DetectingFaces));
              // TODO:(bn) parallel action with an animation here? This will let us span the gap a bit better
              // and buy us more time. Skipping for now
              _state = State::WaitingForFace;
            } else {
              // ...if we've already seen a face, jump straight to turning
              // towards that face and (optionally) saying name.
              CreateFineTuneAction(); // Moves to State:FineTuning
            }
            result = ActionResult::RUNNING;
          }
          
          break;
        }
          
        case State::WaitingForFace:
        {
          result = _action->Update();
          if(_obsFaceID != Vision::UnknownFaceID) {
            // We saw a/the face. Turn towards it and (optionally) say name.
            CreateFineTuneAction(); // Moves to State:FineTuning
            result = ActionResult::RUNNING;
          }
          else if( result != ActionResult::RUNNING && _requireFaceConfirmation ) {
            // the wait action isn't running anymore, we didn't get a face, and we require a face. This is a
            // failure
            result = ActionResult::NO_FACE;
          }
          break;
        }
          
        case State::FineTuning:
        {
          if(nullptr == _action) {
            // No final action, just done.
            result = ActionResult::SUCCESS;
          } else {
            // Wait for final action of fine-tune turning to complete.
             // Create action to say name if enabled and we have a name by now.
            result = _action->Update();
            if(ActionResult::SUCCESS == result && _sayName)
            {
              const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_obsFaceID);
              if(nullptr != face) {
                if( face->GetName().empty() ) {
                  if( _noNameTriggerCallback ) {
                    AnimationTrigger noNameAnim = _noNameTriggerCallback(_robot, _obsFaceID);
                    if( noNameAnim != AnimationTrigger::Count ) {
                      SetAction(new TriggerLiftSafeAnimationAction(_robot, noNameAnim));
                      _state = State::SayingName;
                      result = ActionResult::RUNNING;
                    }
                  }
                }
                else {
                  // we have a name
                  SayTextAction* sayText = new SayTextAction(_robot, face->GetName(), SayTextIntent::Name_Normal);
                  if( _sayNameTriggerCallback ) {
                    AnimationTrigger sayNameAnim = _sayNameTriggerCallback(_robot, _obsFaceID);
                    if( sayNameAnim != AnimationTrigger::Count ) {
                      sayText->SetAnimationTrigger( sayNameAnim );
                    }
                  }
                  SetAction(sayText);
                  _state = State::SayingName;
                  result = ActionResult::RUNNING;
                }
              }
            }
          }
          break;
        }
          
        case State::SayingName:
        {
          if(nullptr == _action) {
            // No say name action, just done
            result = ActionResult::SUCCESS;
          } else {
            // Wait for say name action to finish
            result = _action->Update();
          }
            
          break;
        }
          
      } // switch(_state)

      if( ActionResult::SUCCESS == result && _obsFaceID != Vision::UnknownFaceID ) {
        // tell face world that we have successfully turned towards this face
        _robot.GetFaceWorld().SetTurnedTowardsFace(_obsFaceID);
      }
      
      return result;
      
    } // TurnTowardsFaceAction::CheckIfDone()

#pragma mark ---- TurnTowardsFaceWrapperAction ----

    TurnTowardsFaceWrapperAction::TurnTowardsFaceWrapperAction(Robot& robot,
                                                               IActionRunner* action,
                                                               bool turnBeforeAction,
                                                               bool turnAfterAction,
                                                               Radians maxTurnAngle,
                                                               bool sayName)
    : CompoundActionSequential(robot)
    {
      if( turnBeforeAction ) {
        AddAction( new TurnTowardsLastFacePoseAction(robot, maxTurnAngle, sayName) );
      }
      AddAction(action);
      if( turnAfterAction ) {
        AddAction( new TurnTowardsLastFacePoseAction(robot, maxTurnAngle, sayName) ) ;
      }
      
      // Use the action we're wrapping for the completion info and type
      SetProxyTag(action->GetTag());
    }  
    
#pragma mark ---- WaitAction ----
    
    WaitAction::WaitAction(Robot& robot, f32 waitTimeInSeconds)
    : IAction(robot,
              "WaitSeconds",
              RobotActionType::WAIT,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _waitTimeInSeconds(waitTimeInSeconds)
    , _doneTimeInSeconds(-1.f)
    {
      // Put the wait time with two decimals of precision in the action's name
      char tempBuffer[32];
      snprintf(tempBuffer, 32, "Wait%.2fSeconds", _waitTimeInSeconds);
      SetName(tempBuffer);
    }
    
    ActionResult WaitAction::Init()
    {
      _doneTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _waitTimeInSeconds;
      return ActionResult::SUCCESS;
    }
    
    ActionResult WaitAction::CheckIfDone()
    {
      assert(_doneTimeInSeconds > 0.f);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _doneTimeInSeconds) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::RUNNING;
      }
    }
    
#pragma mark ---- WaitForImagesAction ----
    
    WaitForImagesAction::WaitForImagesAction(Robot& robot, u32 numFrames, VisionMode visionMode, TimeStamp_t afterTimeStamp)
    : IAction(robot,
              "WaitFor" + std::to_string(numFrames) + "Images",
              RobotActionType::WAIT_FOR_IMAGES,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _numFramesToWaitFor(numFrames)
    , _afterTimeStamp(afterTimeStamp)
    , _visionMode(visionMode)
    {
    
    }
    
    ActionResult WaitForImagesAction::Init()
    {
      _numModeFramesSeen = 0;
      
      auto imageProcLambda = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg)
      {
        DEV_ASSERT(ExternalInterface::MessageEngineToGameTag::RobotProcessedImage == msg.GetData().GetTag(),
                   "WaitForImagesAction.MessageTypeNotHandled");
        const ExternalInterface::RobotProcessedImage& imageMsg = msg.GetData().Get_RobotProcessedImage();
        if (imageMsg.timestamp > _afterTimeStamp)
        {
          if (VisionMode::Count == _visionMode)
          {
            ++_numModeFramesSeen;
            PRINT_NAMED_DEBUG("WaitForImagesAction.Callback", "Frame %d of %d for any mode",
                              _numModeFramesSeen, _numFramesToWaitFor);
          }
          else
          {
            for (const auto& mode : imageMsg.visionModes)
            {
              if (mode == _visionMode)
              {
                ++_numModeFramesSeen;
                PRINT_NAMED_DEBUG("WaitForImagesAction.Callback", "Frame %d of %d for mode %s",
                                  _numModeFramesSeen, _numFramesToWaitFor, EnumToString(mode));
                break;
              }
            }
          }
        }
      };
      
      _imageProcSignalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotProcessedImage, imageProcLambda);
      
      return ActionResult::SUCCESS;
    }
    
    ActionResult WaitForImagesAction::CheckIfDone()
    {
      if (_numModeFramesSeen < _numFramesToWaitFor)
      {
        return ActionResult::RUNNING;
      }
      
      // Reset the signalHandler to unsubscribe from the ProcessedImage message in case this action is not
      // immediatly destroyed after completion
      _imageProcSignalHandle.reset();
      return ActionResult::SUCCESS;
    }
    
#pragma mark ---- ReadToolCodeAction ----
    
    ReadToolCodeAction::ReadToolCodeAction(Robot& robot, bool doCalibration)
    : IAction(robot,
              "ReadToolCode",
              RobotActionType::READ_TOOL_CODE,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _doCalibration(doCalibration)
    , _headAndLiftDownAction(robot)
    {
      _toolCodeInfo.code = ToolCode::UnknownTool;
    }
    
    ActionResult ReadToolCodeAction::Init()
    {
      // Put the head and lift down for read
      _headAndLiftDownAction.AddAction(new MoveHeadToAngleAction(_robot, MIN_HEAD_ANGLE));
      _headAndLiftDownAction.AddAction(new MoveLiftToHeightAction(_robot, LIFT_HEIGHT_LOWDOCK, READ_TOOL_CODE_LIFT_HEIGHT_TOL_MM));
      
      _state = State::WaitingToGetInPosition;
      
      _toolReadSignalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotReadToolCode,
         [this] (const AnkiEvent<ExternalInterface::MessageEngineToGame> &msg) {
           _toolCodeInfo = msg.GetData().Get_RobotReadToolCode().info;
           PRINT_CH_INFO("Actions", "ReadToolCodeAction.SignalHandler",
                            "Read tool code: %s", EnumToString(_toolCodeInfo.code));
           this->_state = State::ReadCompleted;
      });
      
      return ActionResult::SUCCESS;
    }
    
    ReadToolCodeAction::~ReadToolCodeAction()
    {
      _robot.GetVisionComponent().EnableMode(VisionMode::ReadingToolCode, false);
      _headAndLiftDownAction.PrepForCompletion();
    }
    
    ActionResult ReadToolCodeAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      switch(_state)
      {
        case State::WaitingToGetInPosition:
        {
          // Wait for head and lift to get into position (i.e. the action to complete)
          result = _headAndLiftDownAction.Update();
          if(ActionResult::SUCCESS == result)
          {
            result = ActionResult::RUNNING; // return value should still be running
            
            Result setCalibResult = _robot.GetVisionComponent().EnableToolCodeCalibration(_doCalibration);
            if(RESULT_OK != setCalibResult) {
              PRINT_CH_INFO("Actions", "ReadToolCodeAction.CheckIfDone.FailedToSetCalibration", "");
              result = ActionResult::FAILED_SETTING_CALIBRATION;
            } else {
              // Tell the VisionSystem thread to check the tool code in the next image it gets.
              // It will disable this mode when it completes.
              _robot.GetVisionComponent().EnableMode(VisionMode::ReadingToolCode, true);
              _state = State::WaitingForRead;
            }
          }
          break;
        }
          
        case State::WaitingForRead:
          // Nothing to do
          break;
          
        case State::ReadCompleted:
          if(_toolCodeInfo.code == ToolCode::UnknownTool) {
            result = ActionResult::UNKNOWN_TOOL_CODE;
          } else {
            result = ActionResult::SUCCESS;
          }
          break;
      }
      
      return result;
    } // CheckIfDone()
    
    void ReadToolCodeAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ReadToolCodeCompleted toolCodeComplete;
      toolCodeComplete.info = _toolCodeInfo;
      completionUnion.Set_readToolCodeCompleted(std::move( toolCodeComplete ));
    }

  }
}
