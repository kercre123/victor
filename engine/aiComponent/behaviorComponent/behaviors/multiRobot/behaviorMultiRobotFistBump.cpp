/**
 * File: BehaviorMultiRobotFistBump.cpp
 *
 * Author: Kevin Yoon
 * Date:   01/23/2017
 *
 * Description: Make Cozmo fist bump with another robot
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/multiRobot/behaviorMultiRobotFistBump.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFistBumpListener.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/multiRobotComponent.h"
#include "engine/faceWorld.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/logging/logging.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {
  
namespace{
// Json parameter keys
static const char* kMaxTimeToLookForFaceKey       = "maxTimeToLookForFace_s";
static const char* kAbortIfNoFaceFoundKey         = "abortIfNoFaceFound";
static const char* kUpdateLastCompletionTimeKey   = "updateLastCompletionTime";
static const char* kIsMasterKey                   = "isMaster";

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
BehaviorMultiRobotFistBump::BehaviorMultiRobotFistBump(const Json::Value& config)
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
, _mrc(nullptr)
, _isMaster(true)
{
  
  JsonTools::GetValueOptional(config, kMaxTimeToLookForFaceKey,     _maxTimeToLookForFace_s);
  JsonTools::GetValueOptional(config, kAbortIfNoFaceFoundKey,       _abortIfNoFaceFound);
  JsonTools::GetValueOptional(config, kUpdateLastCompletionTimeKey, _updateLastCompletionTime);
  JsonTools::GetValueOptional(config, kIsMasterKey,                 _isMaster);

  SubscribeToTags({
    EngineToGameTag::MultiRobotInteractionStateTransition,
  });
}


void BehaviorMultiRobotFistBump::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) 
{
  if (behaviorExternalInterface.HasMultiRobotComponent()) {
    _mrc = &behaviorExternalInterface.GetMultiRobotComponent();
  } else {
    PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.InitBehavior.NoMRC", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorMultiRobotFistBump::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorMultiRobotFistBump::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Disable idle animation
  SmartPushIdleAnimation(behaviorExternalInterface, AnimationTrigger::Count);
  
  _fistBumpRequestCnt = 0;
  _startLookingForFaceTime_s = 0.f;
  _nextGazeChangeTime_s = 0.f;
  _nextGazeChangeIndex = 0;
  _lastTimeOffTreads_s = 0.f;
  
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    _state = State::PutdownObject;
  } else {
    _state = State::GreetSlave;
  }
  _partnerState = State::PutdownObject;

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotFistBump::HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::MultiRobotInteractionStateTransition:
      HandleMultiRobotInteractionStateTransition(event.GetData().Get_MultiRobotInteractionStateTransition(), behaviorExternalInterface);
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorMultiRobotInteractions.HandleWhileActivated.InvalidEvent", "");
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotFistBump::HandleMultiRobotInteractionStateTransition(const ExternalInterface::MultiRobotInteractionStateTransition& msg, BehaviorExternalInterface& behaviorExternalInterface)
{
  _partnerState = (State)msg.newState;
  PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.HandleMultiRobotInteractionStateTransition.NewState", "%d: newState %d",
                      behaviorExternalInterface.GetRobotInfo().GetID(), msg.newState);
}

void BehaviorMultiRobotFistBump::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
  _mrc->SendInteractionStateTransition((int)state);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorMultiRobotFistBump::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  // TODO: Move this to OnBehaviorActivated?
  if (!_mrc) {
    return Status::Complete;
  }

  f32 now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Check if should exit because of pickup
  if (behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads) {
    if (_lastTimeOffTreads_s == 0) {
      _lastTimeOffTreads_s = now;
    } else if (now > _lastTimeOffTreads_s + kMaxPickedupDurationBeforeExit_s) {
      return Status::Complete;
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
        return Status::Running;
      }
    }
  }
  

  switch(_state) {
    case State::PutdownObject:
    {
      if (_isMaster) {
        DelegateIfInControl(new PlaceObjectOnGroundAction());
      }
      SET_STATE(GreetSlave);
      break;
    }

    case State::GreetSlave:
    {
      if (_isMaster) {
        // Turn to robot and greet
        Pose3d slavePose;
        if (_mrc->GetSessionPartnerPose(slavePose) != RESULT_OK) {
          PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.GreetSlave.UnknownPose", "");
          break;
        }
        
        CompoundActionSequential* action = new CompoundActionSequential();
        action->AddAction(new TurnTowardsPoseAction(slavePose));
        action->AddAction(new TriggerAnimationAction(AnimationTrigger::NamedFaceInitialGreeting));
        DelegateIfInControl(action);
      }
      
      SET_STATE(ApproachSlave);
      break;
    }
    case State::ApproachSlave:
    {
      SET_STATE(LookForFace);
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
          _state = State::RequestInitialFistBump;
        }
        break;
      }

      auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
      // Check if face observed very recently
      Pose3d facePose;
      TimeStamp_t lastObservedFaceTime = behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(facePose);
      if (lastObservedFaceTime > 0 && (robotInfo.GetLastMsgTimestamp() - lastObservedFaceTime < kMaxTimeInPastToHaveObservedFace_ms)) {
        DelegateIfInControl(new TurnTowardsLastFacePoseAction());
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
      auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
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
      auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
      if (!robotInfo.GetMoveComponent().IsLiftMoving() &&
          !robotInfo.GetMoveComponent().IsHeadMoving()) {
        _liftWaitingAngle_rad = robotInfo.GetLiftAngle();
        _waitingAccelX_mmps2 = robotInfo.GetHeadAccelData().x;
        _state = State::WaitingForBump;
        if (now - _waitStartTime_s > kMaxTimeForMotorSettling_s) {
          PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.UpdateInternal_Legacy.MotorSettleTimeTooLong", "%f", now - _waitStartTime_s);
        }
      }
      break;
    }
    case State::WaitingForBump:
    {
      auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
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
      NeedActionCompleted();
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
      return Status::Complete;
    }
  }
  
  
  return Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotFistBump::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  robotInfo.GetMoveComponent().EnableLiftPower(true);
  robotInfo.GetMoveComponent().EnableHeadPower(true);
  
  // Make sure trigger is reset if behavior is interrupted
  ResetTrigger(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorMultiRobotFistBump::CheckForBump(const BEIRobotInfo& robotInfo)
{
  bool liftBumped = std::fabsf(robotInfo.GetLiftAngle() - _liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
  bool gyroBumped = std::fabsf(robotInfo.GetHeadGyroData().y) > kGyroYBumpThresh_radps;
  bool accelBumped = std::fabsf(robotInfo.GetHeadAccelData().x - _waitingAccelX_mmps2) > kAccelXBumpThresh_mmps2;

  return liftBumped || gyroBumped || accelBumped;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotFistBump::AddListener(IFistBumpListener* listener)
{
  _fistBumpListeners.insert(listener);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotFistBump::ResetTrigger(bool updateLastCompletionTime)
{
  for (auto &listener : _fistBumpListeners) {
    listener->ResetTrigger(updateLastCompletionTime);
  }
}


} // namespace Cozmo
} // namespace Anki
