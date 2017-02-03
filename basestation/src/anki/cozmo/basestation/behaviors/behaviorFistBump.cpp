/**
 * File: behaviorFistBump.cpp
 *
 * Author: Kevin Yoon
 * Date:   01/23/2017
 *
 * Description: Make Cozmo fist bump with player
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "anki/cozmo/basestation/behaviors/behaviorFistBump.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
  // Json parameter keys
  static const char* kMaxTimeToLookForFaceKey       = "maxTimeToLookForFace_s";
  static const char* kAbortIfNoFaceFoundKey         = "abortIfNoFaceFound";
  static const char* kDoHesitatingInitialRequestKey = "doHesitatingInitialRequest";
  static const char* kWaitTimeoutKey                = "waitForBumpTimeout_s";
  
  // Constants
  static constexpr f32 kLiftAngleBumpThresh_radps   = DEG_TO_RAD(0.5f);
  static constexpr f32 kGyroYBumpThresh_radps       = DEG_TO_RAD(12.f);
  static constexpr u32 kMaxNumAttempts              = 2;
  static constexpr f32 kMaxTimeForMotorSettling_s   = 0.5f; // For DAS warning only
  
  // Looking for face parameters
  static const std::vector<f32> kLookForFaceAngleChanges_rad = {DEG_TO_RAD(-15), DEG_TO_RAD(30)};
  static constexpr f32 kLookForFaceHeadAngle                 = DEG_TO_RAD(35);
  static constexpr f32 kMinTimeBeforeGazeChage_s             = 1.f;
  static constexpr f32 kMaxTimeBeforeGazeChage_s             = 2.f;
  static constexpr u32 kMaxTimeInPastToHaveObservedFace_ms   = 1000;
  
  
BehaviorFistBump::BehaviorFistBump(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
  , _startLookingForFaceTime_s(0.f)
  , _nextGazeChangeTime_s(0.f)
  , _nextGazeChangeIndex(0)
  , _maxTimeToLookForFace_s(3.f)
  , _abortIfNoFaceFound(true)
  , _doHesitatingInitialRequest(false)
  , _waitStartTime_s(0.f)
  , _waitTimeout_s(3.f)
  , _fistBumpRequestCnt(0)
  , _liftWaitingAngle_rad(0.f)
{
  SetDefaultName("FistBump");
  
  const Json::Value& maxTimeToLookForFaceJson = config[kMaxTimeToLookForFaceKey];
  if (!maxTimeToLookForFaceJson.isNull()) {
    _maxTimeToLookForFace_s = maxTimeToLookForFaceJson.asFloat();
  }
  
  const Json::Value& doHesitatingInitialRequestJson = config[kDoHesitatingInitialRequestKey];
  if (!doHesitatingInitialRequestJson.isNull()) {
    _doHesitatingInitialRequest = doHesitatingInitialRequestJson.asBool();
  }

  const Json::Value& abortIfNoFaceFoundJson = config[kAbortIfNoFaceFoundKey];
  if (!abortIfNoFaceFoundJson.isNull()) {
    _abortIfNoFaceFound = abortIfNoFaceFoundJson.asBool();
  }
  
  const Json::Value& waitTimeoutSecJson = config[kWaitTimeoutKey];
  if (!waitTimeoutSecJson.isNull()) {
    _waitTimeout_s = waitTimeoutSecJson.asFloat();
  }

}

bool BehaviorFistBump::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}
  
Result BehaviorFistBump::InitInternal(Robot& robot)
{
  // Disable reactionary behaviors that we don't want interrupting this.
  // (Sometimes when he gets fist bumped too hard it can be interpreted as pickup.)
  std::set<ReactionTrigger> kReactionsToDisable = {
    ReactionTrigger::CubeMoved,
    ReactionTrigger::FacePositionUpdated,
    ReactionTrigger::ObjectPositionUpdated,
    ReactionTrigger::PyramidInitialDetection,
    ReactionTrigger::PetInitialDetection,
    ReactionTrigger::ReturnedToTreads,
    ReactionTrigger::RobotPickedUp,
    ReactionTrigger::StackOfCubesInitialDetection,
    ReactionTrigger::UnexpectedMovement
  };
  SmartDisableReactionTrigger(kReactionsToDisable);
  
  // Turn towards last seen face
  TurnTowardsLastFacePoseAction* turnToFace = new TurnTowardsLastFacePoseAction(robot);
  turnToFace->SetRequireFaceConfirmation(true);
  StartActing(turnToFace, [this](ActionResult result) {
    if (result == ActionResult::NO_FACE) {
      _startLookingForFaceTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _state = State::LookingForFace;
    } else {
      _state = State::RequestInitialFistBump;
    }
  });
  
  _fistBumpRequestCnt = 0;
  _startLookingForFaceTime_s = 0.f;
  _nextGazeChangeTime_s = 0.f;
  _nextGazeChangeIndex = 0;
  
  _state = State::RequestInitialFistBump;

  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorFistBump::UpdateInternal(Robot& robot)
{
  // If no action currently running, return Running, unless the state is
  // WaitingForMotorsToSettle or WaitingForBump in which case we loop the idle animation.
  switch(_state) {
    case State::WaitingForMotorsToSettle:
    case State::WaitingForBump:
    {
      // Time to play idle anim?
      if (!IsActing()) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpIdle));
      }
      break;
    }
    default:
    {
      if (IsActing()) {
        return Status::Running;
      }
    }
  }
  
  f32 now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch(_state) {
    case State::LookingForFace:
    {
      // Check if time to stop looking for face
      if (now > _startLookingForFaceTime_s + _maxTimeToLookForFace_s) {
        if (_abortIfNoFaceFound) {
          _state = State::Complete;
        } else {
          _state = State::RequestInitialFistBump;
        }
        break;
      }
      
      // Check if face observed very recently
      Pose3d facePose;
      TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);
      if (lastObservedFaceTime > 0 && (robot.GetLastMsgTimestamp() - lastObservedFaceTime < kMaxTimeInPastToHaveObservedFace_ms)) {
        StartActing(new TurnTowardsLastFacePoseAction(robot));
        _state = State::RequestInitialFistBump;
        break;
      }
      
      // Check if time to adjust gaze
      if (now > _nextGazeChangeTime_s) {
        PanAndTiltAction* ptAction = new PanAndTiltAction(robot, kLookForFaceAngleChanges_rad[_nextGazeChangeIndex], kLookForFaceHeadAngle, false, true);
        StartActing(ptAction);
        
        // Set next gaze change time
        _nextGazeChangeTime_s = now + GetRNG().RandDblInRange(kMinTimeBeforeGazeChage_s, kMaxTimeBeforeGazeChage_s);
        if (++_nextGazeChangeIndex >= kLookForFaceAngleChanges_rad.size()) {
          _nextGazeChangeIndex = 0;
        }
      }
      
      break;
    }
    case State::RequestInitialFistBump:
    {
      if (_doHesitatingInitialRequest) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpRequestLong));
      } else {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpRequestOnce));
      }
      _state = State::RequestingFistBump;
      break;
    }
    case State::RequestingFistBump:
    {
      _waitStartTime_s = now;
      robot.GetMoveComponent().EnableLiftPower(false);
      robot.GetMoveComponent().EnableHeadPower(false);
      _state = State::WaitingForMotorsToSettle;
      break;
    }
    case State::WaitingForMotorsToSettle:
    {
      if (!robot.GetMoveComponent().IsLiftMoving() &&
          !robot.GetMoveComponent().IsHeadMoving()) {
        _liftWaitingAngle_rad = robot.GetLiftAngle();
        _state = State::WaitingForBump;
        if (now - _waitStartTime_s > kMaxTimeForMotorSettling_s) {
          PRINT_NAMED_WARNING("BehaviorFistBump.UpdateInternal.MotorSettleTimeTooLong", "%f", now - _waitStartTime_s);
        }
      }
      break;
    }
    case State::WaitingForBump:
    {
      if (CheckForBump(robot)) {
        StopActing();  // Stop the idle anim
        robot.GetMoveComponent().EnableLiftPower(true);
        robot.GetMoveComponent().EnableHeadPower(true);
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpSuccess));
        _state = State::Complete;
      }
      
      // Check for wait timeout and retry request if necessary
      if (now >= _waitStartTime_s + _waitTimeout_s) {
        StopActing();  // Stop idle anim
        robot.GetMoveComponent().EnableLiftPower(true);
        robot.GetMoveComponent().EnableHeadPower(true);
        if (++_fistBumpRequestCnt < kMaxNumAttempts) {
          StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpRequestRetry));
          _state = State::RequestingFistBump;
        } else {
          StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpLeftHanging));
          _state = State::Complete;
        }
      }
      
      break;
    }
    case State::Complete:
    {
      BehaviorObjectiveAchieved(BehaviorObjective::FistBumped);
      return Status::Complete;
    }
  }
  
  
  return Status::Running;
}
  
void BehaviorFistBump::StopInternal(Robot& robot)
{
  robot.GetMoveComponent().EnableLiftPower(true);
  robot.GetMoveComponent().EnableHeadPower(true);
}

  
bool BehaviorFistBump::CheckForBump(const Robot& robot)
{
  bool liftBumped = std::fabsf(robot.GetLiftAngle() - _liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
  bool gyroBumped = std::fabsf(robot.GetHeadGyroData().y) > kGyroYBumpThresh_radps;
  
  return liftBumped || gyroBumped;
}



} // namespace Cozmo
} // namespace Anki
