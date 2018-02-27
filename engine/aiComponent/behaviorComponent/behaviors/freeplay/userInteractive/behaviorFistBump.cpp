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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorFistBump.h"
#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFistBumpListener.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{
// Json parameter keys
static const char* kMaxTimeToLookForFaceKey       = "maxTimeToLookForFace_s";
static const char* kAbortIfNoFaceFoundKey         = "abortIfNoFaceFound";
static const char* kUpdateLastCompletionTimeKey   = "updateLastCompletionTime";

// Constants
static constexpr f32 kLiftAngleBumpThresh_radps   = DEG_TO_RAD(0.5f);
static constexpr f32 kGyroYBumpThresh_radps       = DEG_TO_RAD(10.f);
static constexpr f32 kAccelXBumpThresh_mmps2      = 4000.f;
static constexpr u32 kMaxNumAttempts              = 2;
static constexpr f32 kMaxTimeForMotorSettling_s   = 0.5f; // For DAS warning only
static constexpr f32 kMaxPickedupDurationBeforeExit_s = 1.f;

// Looking for face parameters
static const std::vector<f32> kLookForFaceAngleChanges_rad = {DEG_TO_RAD(-15), DEG_TO_RAD(30)};
static constexpr f32 kLookForFaceHeadAngle                 = DEG_TO_RAD(35);
static constexpr f32 kMinTimeBeforeGazeChange_s            = 1.f;
static constexpr f32 kMaxTimeBeforeGazeChange_s            = 2.f;
static constexpr u32 kMaxTimeInPastToHaveObservedFace_ms   = 1000;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFistBump::BehaviorFistBump(const Json::Value& config)
: ICozmoBehavior(config)
, _startLookingForFaceTime_s(0.f)
, _nextGazeChangeTime_s(0.f)
, _nextGazeChangeIndex(0)
, _maxTimeToLookForFace_s(0.f)
, _abortIfNoFaceFound(true)
, _waitStartTime_s(0.f)
, _fistBumpRequestCnt(0)
, _liftWaitingAngle_rad(0.f)
, _lastTimeOffTreads_s(0.f)
, _updateLastCompletionTime(false)
{
  
  JsonTools::GetValueOptional(config, kMaxTimeToLookForFaceKey,     _maxTimeToLookForFace_s);
  JsonTools::GetValueOptional(config, kAbortIfNoFaceFoundKey,       _abortIfNoFaceFound);
  JsonTools::GetValueOptional(config, kUpdateLastCompletionTimeKey, _updateLastCompletionTime);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFistBump::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::OnBehaviorActivated()
{
  // Disable idle animation
  SmartPushIdleAnimation(AnimationTrigger::Count);
  
  _fistBumpRequestCnt = 0;
  _startLookingForFaceTime_s = 0.f;
  _nextGazeChangeTime_s = 0.f;
  _nextGazeChangeIndex = 0;
  _lastTimeOffTreads_s = 0.f;
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    _state = State::PutdownObject;
  } else {
    _state = State::LookForFace;
  }

  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  f32 now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Check if should exit because of pickup
  if (GetBEI().GetOffTreadsState() != OffTreadsState::OnTreads) {
    if (_lastTimeOffTreads_s == 0) {
      _lastTimeOffTreads_s = now;
    } else if (now > _lastTimeOffTreads_s + kMaxPickedupDurationBeforeExit_s) {
      CancelSelf();
      return;
    }
  } else {
    _lastTimeOffTreads_s = 0;
  }
  
  
  // If no action currently running, return Running, unless the state is
  // WaitingForMotorsToSettle or WaitingForBump in which case we loop the idle animation.
  switch(_state) {
    case State::WaitingForMotorsToSettle:
    case State::WaitingForBump:
    {
      break;
    }
    default:
    {
      if (IsControlDelegated()) {
        return;
      }
    }
  }
  

  switch(_state) {
    case State::PutdownObject:
    {
      DelegateIfInControl(new PlaceObjectOnGroundAction());
      _state = State::LookForFace;
      break;
    }
    case State::LookForFace:
    {
      // Turn towards last seen face
      TurnTowardsLastFacePoseAction* turnToFace = new TurnTowardsLastFacePoseAction();
      turnToFace->SetRequireFaceConfirmation(true);
      DelegateIfInControl(turnToFace, [this](ActionResult result) {
        if (result == ActionResult::NO_FACE) {
          _startLookingForFaceTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          _state = State::LookingForFace;
        } else {
          ResetFistBumpTimer();
          _state = State::RequestInitialFistBump;
        }
      });
      break;
    }
    case State::LookingForFace:
    {
      // Check if time to stop looking for face
      if (now > _startLookingForFaceTime_s + _maxTimeToLookForFace_s) {
        if (_abortIfNoFaceFound) {
          _state = State::Complete;
        } else {
          ResetFistBumpTimer();
          _state = State::RequestInitialFistBump;
        }
        break;
      }

      auto& robotInfo = GetBEI().GetRobotInfo();
      // Check if face observed very recently
      Pose3d facePose;
      TimeStamp_t lastObservedFaceTime = GetBEI().GetFaceWorld().GetLastObservedFace(facePose);
      if (lastObservedFaceTime > 0 && (robotInfo.GetLastMsgTimestamp() - lastObservedFaceTime < kMaxTimeInPastToHaveObservedFace_ms)) {
        DelegateIfInControl(new TurnTowardsLastFacePoseAction());
        ResetFistBumpTimer();
        _state = State::RequestInitialFistBump;
        break;
      }
      
      // Check if time to adjust gaze
      if (now > _nextGazeChangeTime_s) {
        PanAndTiltAction* ptAction = new PanAndTiltAction(kLookForFaceAngleChanges_rad[_nextGazeChangeIndex], kLookForFaceHeadAngle, false, true);
        DelegateIfInControl(ptAction);
        
        // Set next gaze change time
        _nextGazeChangeTime_s = now + Util::numeric_cast<float>(GetRNG().RandDblInRange(kMinTimeBeforeGazeChange_s, kMaxTimeBeforeGazeChange_s));
        if (++_nextGazeChangeIndex >= kLookForFaceAngleChanges_rad.size()) {
          _nextGazeChangeIndex = 0;
        }
      }
      
      break;
    }
    case State::RequestInitialFistBump:
    {
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpRequestOnce));
      _state = State::RequestingFistBump;
      break;
    }
    case State::RequestingFistBump:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      _waitStartTime_s = now;
      robotInfo.GetMoveComponent().EnableLiftPower(false);
      robotInfo.GetMoveComponent().EnableHeadPower(false);
      
      // Play idle anim
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpIdle));
      
      _state = State::WaitingForMotorsToSettle;
      break;
    }
    case State::WaitingForMotorsToSettle:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      if (!robotInfo.GetMoveComponent().IsLiftMoving() &&
          !robotInfo.GetMoveComponent().IsHeadMoving()) {
        _liftWaitingAngle_rad = robotInfo.GetLiftAngle();
        _waitingAccelX_mmps2 = robotInfo.GetHeadAccelData().x;
        _state = State::WaitingForBump;
        if (now - _waitStartTime_s > kMaxTimeForMotorSettling_s) {
          PRINT_NAMED_WARNING("BehaviorFistBump.UpdateInternal_Legacy.MotorSettleTimeTooLong", "%f", now - _waitStartTime_s);
        }
      }
      break;
    }
    case State::WaitingForBump:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      if (CheckForBump(robotInfo)) {
        CancelDelegates();  // Stop the idle anim
        robotInfo.GetMoveComponent().EnableLiftPower(true);
        robotInfo.GetMoveComponent().EnableHeadPower(true);
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpSuccess));
        _state = State::CompleteSuccess;
      }
      
      // When idle anim is complete, retry or fail
      if (!IsControlDelegated()) {
        robotInfo.GetMoveComponent().EnableLiftPower(true);
        robotInfo.GetMoveComponent().EnableHeadPower(true);
        if (++_fistBumpRequestCnt < kMaxNumAttempts) {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpRequestRetry));
          _state = State::RequestingFistBump;
        } else {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpLeftHanging));
          _state = State::CompleteFail;
        }
      }
      
      break;
    }
    case State::CompleteSuccess:
    {
      // Fall through
    }
    case State::CompleteFail:
    {
      // Should only be sending FistBumpSuccess or FistBumpLeftHanging if this not the sparks Fist bump
      // since we don't want the sparks fist bumps to reset the cooldown timer in the trigger strategy.
      BehaviorObjectiveAchieved(_state == State::CompleteSuccess ? BehaviorObjective::FistBumpSuccess : BehaviorObjective::FistBumpLeftHanging);
      
      // Fall through
    }
    case State::Complete:
    {
      ResetTrigger(_updateLastCompletionTime);
      BehaviorObjectiveAchieved(BehaviorObjective::FistBumpComplete);
      CancelSelf();
      return;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetMoveComponent().EnableLiftPower(true);
  robotInfo.GetMoveComponent().EnableHeadPower(true);
  
  // Make sure trigger is reset if behavior is interrupted
  ResetTrigger(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFistBump::CheckForBump(const BEIRobotInfo& robotInfo)
{
  bool liftBumped = std::fabsf(robotInfo.GetLiftAngle() - _liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
  bool gyroBumped = std::fabsf(robotInfo.GetHeadGyroData().y) > kGyroYBumpThresh_radps;
  bool accelBumped = std::fabsf(robotInfo.GetHeadAccelData().x - _waitingAccelX_mmps2) > kAccelXBumpThresh_mmps2;

  return liftBumped || gyroBumped || accelBumped;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::AddListener(IFistBumpListener* listener)
{
  _fistBumpListeners.insert(listener);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::ResetTrigger(bool updateLastCompletionTime)
{
  for (auto &listener : _fistBumpListeners) {
    listener->ResetTrigger(updateLastCompletionTime);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::ResetFistBumpTimer() const
{
  GetBEI().GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::FistBump ).Reset();
}


} // namespace Cozmo
} // namespace Anki
