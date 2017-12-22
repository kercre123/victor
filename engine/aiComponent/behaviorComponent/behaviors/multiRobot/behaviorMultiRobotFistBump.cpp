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
#include "engine/actions/driveToActions.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/multiRobotComponent.h"
#include "engine/faceWorld.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/logging/logging.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)
#define STATE_NAME(s) #s
#define SET_STATE_WHEN_PARTNER_STATE(s, ps) SetStateWhenPartnerState(State::s, #s, State::ps)

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
, _pendingStateName("")
, _unknownPoseCount(0)
{
  
  JsonTools::GetValueOptional(config, kMaxTimeToLookForFaceKey,     _maxTimeToLookForFace_s);
  JsonTools::GetValueOptional(config, kAbortIfNoFaceFoundKey,       _abortIfNoFaceFound);
  JsonTools::GetValueOptional(config, kUpdateLastCompletionTimeKey, _updateLastCompletionTime);
  JsonTools::GetValueOptional(config, kIsMasterKey,                 _isMaster);

  SubscribeToTags({{
    EngineToGameTag::MultiRobotInteractionStateTransition,
    EngineToGameTag::MultiRobotSessionEnded,
  }});
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
  
  _sessionEnded = _mrc == nullptr;
  _unknownPoseCount = 0;

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
    case ExternalInterface::MessageEngineToGameTag::MultiRobotSessionEnded:
      HandleMultiRobotSessionEnded(event.GetData().Get_MultiRobotSessionEnded(), behaviorExternalInterface);
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

void BehaviorMultiRobotFistBump::HandleMultiRobotSessionEnded(const ExternalInterface::MultiRobotSessionEnded& msg, BehaviorExternalInterface& behaviorExternalInterface)
{
  _sessionEnded = true;
  PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.HandleMultiRobotSessionEnded", "%d",
                      behaviorExternalInterface.GetRobotInfo().GetID());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void BehaviorMultiRobotFistBump::SetState_internal(State state, const std::string& stateName)
{
  PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.SetState.state", "%d: newState %s",
                      _isMaster, stateName.c_str());
  _state = state;
  SetDebugStateName(stateName);
  _mrc->SendInteractionStateTransition((int)state);
}

  
void BehaviorMultiRobotFistBump::SetStateWhenPartnerState(State state, const std::string& stateName, State partnerState)
{
  if (_partnerState >= partnerState) {
    SetState_internal(state, stateName);
  } else {
    _pendingState = state;
    _pendingStateName = stateName;
    _gatingPartnerState = partnerState;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorMultiRobotFistBump::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if (_sessionEnded) {
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
  
  // Check for pending state transitions
  if (_pendingStateName != "") {
    if (_partnerState >= _gatingPartnerState) {
      PRINT_NAMED_WARNING("MultiRobotFistBump.Update.PendingStateTransitionComplete", "NewState: %s", _pendingStateName.c_str());
      SetState_internal(_pendingState, _pendingStateName);
      _pendingStateName = "";
    } else {
      return Status::Running;
    }
  }
  
  
  // If no action currently running, return Running, unless the state is
  // WaitingForMotorsToSettle or WaitingForBump in which case we loop the idle animation.
  switch(_state) {
    case State::WaitingForMotorsToSettle:
    case State::WaitingForBump:
    case State::DrivingIntoFist:
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
  
  // RobotInfo ref for convenience
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  
  // Pick pose 15cm from partner robot and go there
  Pose3d partnerPose;
  if (_mrc->GetSessionPartnerPose(partnerPose) != RESULT_OK) {
    PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.Update.UnknownPose", "");
    if (++_unknownPoseCount > 50 || !_mrc->IsInSession()) {
      return Status::Complete;
    }
    return Status::Running;
  }
  _unknownPoseCount = 0;

  

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
        CompoundActionSequential* action = new CompoundActionSequential();
        action->AddAction(new TurnTowardsPoseAction(partnerPose));
        action->AddAction(new TriggerAnimationAction(AnimationTrigger::NamedFaceInitialGreeting));
        DelegateIfInControl(action, [this](const ExternalInterface::RobotCompletedAction& result) {
          SET_STATE(ApproachSlave);
        });
      } else {
        SET_STATE_WHEN_PARTNER_STATE(ApproachSlave, ApproachSlave);
      }
      break;
    }
    case State::ApproachSlave:
    {
      if (_isMaster) {
        
        const Pose3d masterPose = behaviorExternalInterface.GetRobotInfo().GetPose().GetWithRespectToRoot();
        
        const auto& masterPoint = masterPose.GetTranslation();
        const auto& slavePoint = partnerPose.GetTranslation();
        f32 dx =  slavePoint.x() - masterPoint.x();
        f32 dy =  slavePoint.y() - masterPoint.y();
        const f32 goalDistFromSlave = 130;
        
        const f32 angleToSlave = atan2(dy,dx);
        
        Pose3d goalPose(angleToSlave, Z_AXIS_3D(), { slavePoint.x() - goalDistFromSlave * cos(angleToSlave),
                                                     slavePoint.y() - goalDistFromSlave * sin(angleToSlave),
                                                     slavePoint.z()
        }, robotInfo.GetPose().GetParent() );

        DelegateIfInControl(new DriveToPoseAction(goalPose, false), [this](const ExternalInterface::RobotCompletedAction& result) {
          SET_STATE(RequestInitialFistBump);
        });
      } else {
        // Turn to master after he greeted you
        CompoundActionSequential* action = new CompoundActionSequential();
        action->AddAction(new TurnTowardsPoseAction(partnerPose));
        action->AddAction(new TriggerAnimationAction(AnimationTrigger::CodeLabHappy));
        DelegateIfInControl(action);
        SET_STATE_WHEN_PARTNER_STATE(PrepareToBumpFist, WaitingForMotorsToSettle);
      }

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
      SET_STATE(RequestingFistBump);
      break;
    }
    case State::RequestingFistBump:
    {
      _waitStartTime_s = now;
      robotInfo.GetMoveComponent().EnableLiftPower(false);
      robotInfo.GetMoveComponent().EnableHeadPower(false);
      
      // Play idle anim
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpIdle));
      
      SET_STATE(WaitingForMotorsToSettle);
      break;
    }
    case State::WaitingForMotorsToSettle:
    {
      if (!robotInfo.GetMoveComponent().IsLiftMoving() &&
          !robotInfo.GetMoveComponent().IsHeadMoving()) {
        _liftWaitingAngle_rad = robotInfo.GetLiftAngle();
        _waitingAccelX_mmps2 = robotInfo.GetHeadAccelData().x;
        SET_STATE(WaitingForBump);
        if (now - _waitStartTime_s > kMaxTimeForMotorSettling_s) {
          PRINT_NAMED_WARNING("BehaviorMultiRobotFistBump.UpdateInternal_Legacy.MotorSettleTimeTooLong", "%f", now - _waitStartTime_s);
        }
      }
      break;
    }
    case State::WaitingForBump:
    {
      if (CheckForBump(robotInfo)) {
        CancelDelegates();  // Stop the idle anim
        robotInfo.GetMoveComponent().EnableLiftPower(true);
        robotInfo.GetMoveComponent().EnableHeadPower(true);
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpSuccess));
        SET_STATE(CompleteSuccess);
      }
      
      // When idle anim is complete, retry or fail
      if (!IsControlDelegated()) {
        robotInfo.GetMoveComponent().EnableLiftPower(true);
        robotInfo.GetMoveComponent().EnableHeadPower(true);
        if (++_fistBumpRequestCnt < kMaxNumAttempts) {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpRequestRetry));
          SET_STATE(RequestingFistBump);
        } else {
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FistBumpLeftHanging));
          SET_STATE(CompleteFail);
        }
      }
      
      break;
    }
      
    // SLAVE-ONLY STATES
    case State::PrepareToBumpFist:
    {
      CompoundActionSequential* action = new CompoundActionSequential();
      action->AddAction(new MoveLiftToHeightAction(80));
      DelegateIfInControl(action, [this](const BehaviorExternalInterface& behaviorExternalInterface){
        auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
        robotInfo.GetMoveComponent().EnableLiftPower(false);
        SET_STATE(BumpFist);
      });

      break;
    }
    case State::BumpFist:
    {
      // Wait for motor to settle then drive forward
      if (!robotInfo.GetMoveComponent().IsLiftMoving()) {
        _liftWaitingAngle_rad = robotInfo.GetLiftAngle();
        DelegateIfInControl(new DriveStraightAction(100));
        SET_STATE(DrivingIntoFist);
      }
      break;
    }
    case State::DrivingIntoFist:
    {
      if (IsControlDelegated()) {
        // Still driving forward, hopefully into a robot fist
        bool liftBumped = std::fabsf(robotInfo.GetLiftAngle() - _liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
        if (liftBumped) {
          CancelDelegates();  // Stop the DriveStraightAction
          robotInfo.GetMoveComponent().EnableLiftPower(true);
          CompoundActionSequential* action = new CompoundActionSequential();
          action->AddAction(new DriveStraightAction(-30, 100));
          action->AddAction(new TriggerAnimationAction(AnimationTrigger::FistBumpSuccess));
          DelegateIfInControl(action);
          SET_STATE(CompleteSuccess);
        }
      } else {
        // Stopped driving forward and no fist was bumped
        CompoundActionSequential* action = new CompoundActionSequential();
          action->AddAction(new DriveStraightAction(-30, 30));
          action->AddAction(new TriggerAnimationAction(AnimationTrigger::FistBumpLeftHanging));
        DelegateIfInControl(action);
        SET_STATE(CompleteFail);
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
