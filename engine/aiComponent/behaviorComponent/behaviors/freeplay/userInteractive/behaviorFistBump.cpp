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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFistBumpListener.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

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
BehaviorFistBump::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  // default values
  maxTimeToLookForFace_s   = 0.f;
  abortIfNoFaceFound       = true;
  updateLastCompletionTime = false;

  // load alt values in from JSON
  JsonTools::GetValueOptional(config, kMaxTimeToLookForFaceKey,     maxTimeToLookForFace_s);
  JsonTools::GetValueOptional(config, kAbortIfNoFaceFoundKey,       abortIfNoFaceFound);
  JsonTools::GetValueOptional(config, kUpdateLastCompletionTimeKey, updateLastCompletionTime);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFistBump::DynamicVariables::DynamicVariables()
{
  state = State::LookForFace;

  startLookingForFaceTime_s = 0.f;
  nextGazeChangeTime_s      = 0.f;
  nextGazeChangeIndex       = 0;
  waitStartTime_s           = 0.f;
  fistBumpRequestCnt        = 0;
  liftWaitingAngle_rad      = 0.f;
  lastTimeOffTreads_s       = 0.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFistBump::BehaviorFistBump(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMaxTimeToLookForFaceKey,
    kAbortIfNoFaceFoundKey,
    kUpdateLastCompletionTimeKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
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
  _dVars = DynamicVariables();

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    _dVars.state = State::PutdownObject;
  } else {
    _dVars.state = State::LookForFace;
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
    if (_dVars.lastTimeOffTreads_s == 0) {
      _dVars.lastTimeOffTreads_s = now;
    } else if (now > _dVars.lastTimeOffTreads_s + kMaxPickedupDurationBeforeExit_s) {
      CancelSelf();
      return;
    }
  } else {
    _dVars.lastTimeOffTreads_s = 0;
  }
  
  
  // If no action currently running, return Running, unless the state is
  // WaitingForMotorsToSettle or WaitingForBump in which case we loop the idle animation.
  switch(_dVars.state) {
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
  

  switch(_dVars.state) {
    case State::PutdownObject:
    {
      DelegateIfInControl(new PlaceObjectOnGroundAction());
      _dVars.state = State::LookForFace;
      break;
    }
    case State::LookForFace:
    {
      // Turn towards last seen face
      TurnTowardsLastFacePoseAction* turnToFace = new TurnTowardsLastFacePoseAction();
      turnToFace->SetRequireFaceConfirmation(true);
      DelegateIfInControl(turnToFace, [this](ActionResult result) {
        if (result == ActionResult::NO_FACE) {
          _dVars.startLookingForFaceTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          _dVars.state = State::LookingForFace;
        } else {
          ResetFistBumpTimer();
          _dVars.state = State::RequestInitialFistBump;
        }
      });
      break;
    }
    case State::LookingForFace:
    {
      // Check if time to stop looking for face
      if (now > _dVars.startLookingForFaceTime_s + _iConfig.maxTimeToLookForFace_s) {
        if (_iConfig.abortIfNoFaceFound) {
          _dVars.state = State::Complete;
        } else {
          ResetFistBumpTimer();
          _dVars.state = State::RequestInitialFistBump;
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
        _dVars.state = State::RequestInitialFistBump;
        break;
      }
      
      // Check if time to adjust gaze
      if (now > _dVars.nextGazeChangeTime_s) {
        PanAndTiltAction* ptAction = new PanAndTiltAction(kLookForFaceAngleChanges_rad[_dVars.nextGazeChangeIndex], kLookForFaceHeadAngle, false, true);
        DelegateIfInControl(ptAction);
        
        // Set next gaze change time
        _dVars.nextGazeChangeTime_s = now + Util::numeric_cast<float>(GetRNG().RandDblInRange(kMinTimeBeforeGazeChange_s, kMaxTimeBeforeGazeChange_s));
        if (++_dVars.nextGazeChangeIndex >= kLookForFaceAngleChanges_rad.size()) {
          _dVars.nextGazeChangeIndex = 0;
        }
      }
      
      break;
    }
    case State::RequestInitialFistBump:
    {
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpRequestOnce));
      _dVars.state = State::RequestingFistBump;
      break;
    }
    case State::RequestingFistBump:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      _dVars.waitStartTime_s = now;
      robotInfo.GetMoveComponent().EnableLiftPower(false);
      robotInfo.GetMoveComponent().EnableHeadPower(false);
      
      // Play idle anim
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpIdle));
      
      _dVars.state = State::WaitingForMotorsToSettle;
      break;
    }
    case State::WaitingForMotorsToSettle:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      if (!robotInfo.GetMoveComponent().IsLiftMoving() &&
          !robotInfo.GetMoveComponent().IsHeadMoving()) {
        _dVars.liftWaitingAngle_rad = robotInfo.GetLiftAngle();
        _dVars.waitingAccelX_mmps2 = robotInfo.GetHeadAccelData().x;
        _dVars.state = State::WaitingForBump;
        if (now - _dVars.waitStartTime_s > kMaxTimeForMotorSettling_s) {
          PRINT_NAMED_WARNING("BehaviorFistBump.UpdateInternal_Legacy.MotorSettleTimeTooLong", "%f", now - _dVars.waitStartTime_s);
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
        _dVars.state = State::CompleteSuccess;
      }
      
      // When idle anim is complete, retry or fail
      if (!IsControlDelegated()) {
        robotInfo.GetMoveComponent().EnableLiftPower(true);
        robotInfo.GetMoveComponent().EnableHeadPower(true);
        if (++_dVars.fistBumpRequestCnt < kMaxNumAttempts) {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpRequestRetry));
          _dVars.state = State::RequestingFistBump;
        } else {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpLeftHanging));
          _dVars.state = State::CompleteFail;
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
      BehaviorObjectiveAchieved(_dVars.state == State::CompleteSuccess ?
                                BehaviorObjective::FistBumpSuccess :
                                BehaviorObjective::FistBumpLeftHanging);

      if(GetBEI().HasMoodManager()){
        auto& moodManager = GetBEI().GetMoodManager();
        moodManager.TriggerEmotionEvent(_dVars.state == State::CompleteSuccess ?
                                        "FistBumpSuccess" :
                                        "FistBumpLeftHanging",
                                        MoodManager::GetCurrentTimeInSeconds());
      }

      
      // Fall through
    }
    case State::Complete:
    {
      ResetTrigger(_iConfig.updateLastCompletionTime);
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
  bool liftBumped = std::fabsf(robotInfo.GetLiftAngle() - _dVars.liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
  bool gyroBumped = std::fabsf(robotInfo.GetHeadGyroData().y) > kGyroYBumpThresh_radps;
  bool accelBumped = std::fabsf(robotInfo.GetHeadAccelData().x - _dVars.waitingAccelX_mmps2) > kAccelXBumpThresh_mmps2;

  return liftBumped || gyroBumped || accelBumped;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::AddListener(IFistBumpListener* listener)
{
  _iConfig.fistBumpListeners.insert(listener);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFistBump::ResetTrigger(bool updateLastCompletionTime)
{
  for (auto &listener : _iConfig.fistBumpListeners) {
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
