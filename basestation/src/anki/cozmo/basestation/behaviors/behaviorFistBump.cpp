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
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iFistBumpListener.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

  CONSOLE_VAR(bool, kCanHiccupWhileFistBumping, "Hiccups", true);
  
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
  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersFistBumpArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersFistBumpArray),
              "Reaction triggers duplicate or non-sequential");
  
}
  
  
BehaviorFistBump::BehaviorFistBump(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
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
  SetDefaultName("FistBump");
  
  JsonTools::GetValueOptional(config, kMaxTimeToLookForFaceKey,     _maxTimeToLookForFace_s);
  JsonTools::GetValueOptional(config, kAbortIfNoFaceFoundKey,       _abortIfNoFaceFound);
  JsonTools::GetValueOptional(config, kUpdateLastCompletionTimeKey, _updateLastCompletionTime);
}

bool BehaviorFistBump::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}
  
Result BehaviorFistBump::InitInternal(Robot& robot)
{
  // Disable reactionary behaviors that we don't want interrupting this.
  // (Sometimes when he gets fist bumped too hard it can be interpreted as pickup.)
  if(!kCanHiccupWhileFistBumping)
  {
    SMART_DISABLE_REACTION_DEV_ONLY(GetName(), ReactionTrigger::Hiccup);
  }

  SmartDisableReactionsWithLock(GetName(), kAffectTriggersFistBumpArray);

  // Disable idle animation
  robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
  
  _fistBumpRequestCnt = 0;
  _startLookingForFaceTime_s = 0.f;
  _nextGazeChangeTime_s = 0.f;
  _nextGazeChangeIndex = 0;
  _lastTimeOffTreads_s = 0.f;
  
  if (robot.IsCarryingObject()) {
    _state = State::PutdownObject;
  } else {
    _state = State::LookForFace;
  }

  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorFistBump::UpdateInternal(Robot& robot)
{
  f32 now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Check if should exit because of pickup
  if (robot.GetOffTreadsState() != OffTreadsState::OnTreads) {
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
      if (IsActing()) {
        return Status::Running;
      }
    }
  }
  

  switch(_state) {
    case State::PutdownObject:
    {
      StartActing(new PlaceObjectOnGroundAction(robot));
      _state = State::LookForFace;
      break;
    }
    case State::LookForFace:
    {
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
        _nextGazeChangeTime_s = now + Util::numeric_cast<float>(GetRNG().RandDblInRange(kMinTimeBeforeGazeChange_s, kMaxTimeBeforeGazeChange_s));
        if (++_nextGazeChangeIndex >= kLookForFaceAngleChanges_rad.size()) {
          _nextGazeChangeIndex = 0;
        }
      }
      
      break;
    }
    case State::RequestInitialFistBump:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpRequestOnce));
      _state = State::RequestingFistBump;
      break;
    }
    case State::RequestingFistBump:
    {
      _waitStartTime_s = now;
      robot.GetMoveComponent().EnableLiftPower(false);
      robot.GetMoveComponent().EnableHeadPower(false);
      
      // Play idle anim
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpIdle));
      
      _state = State::WaitingForMotorsToSettle;
      break;
    }
    case State::WaitingForMotorsToSettle:
    {
      if (!robot.GetMoveComponent().IsLiftMoving() &&
          !robot.GetMoveComponent().IsHeadMoving()) {
        _liftWaitingAngle_rad = robot.GetLiftAngle();
        _waitingAccelX_mmps2 = robot.GetHeadAccelData().x;
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
        _state = State::CompleteSuccess;
      }
      
      // When idle anim is complete, retry or fail
      if (!IsActing()) {
        robot.GetMoveComponent().EnableLiftPower(true);
        robot.GetMoveComponent().EnableHeadPower(true);
        if (++_fistBumpRequestCnt < kMaxNumAttempts) {
          StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpRequestRetry));
          _state = State::RequestingFistBump;
        } else {
          StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FistBumpLeftHanging));
          _state = State::CompleteFail;
        }
      }
      
      break;
    }
    case State::CompleteSuccess:
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
  
void BehaviorFistBump::StopInternal(Robot& robot)
{
  robot.GetMoveComponent().EnableLiftPower(true);
  robot.GetMoveComponent().EnableHeadPower(true);
  
  robot.GetAnimationStreamer().PopIdleAnimation();

  // Make sure trigger is reset if behavior is interrupted
  ResetTrigger(false);
}

  
bool BehaviorFistBump::CheckForBump(const Robot& robot)
{
  bool liftBumped = std::fabsf(robot.GetLiftAngle() - _liftWaitingAngle_rad) > kLiftAngleBumpThresh_radps;
  bool gyroBumped = std::fabsf(robot.GetHeadGyroData().y) > kGyroYBumpThresh_radps;
  bool accelBumped = std::fabsf(robot.GetHeadAccelData().x - _waitingAccelX_mmps2) > kAccelXBumpThresh_mmps2;

  return liftBumped || gyroBumped || accelBumped;
}


void BehaviorFistBump::AddListener(IFistBumpListener* listener)
{
  _fistBumpListeners.insert(listener);
}

void BehaviorFistBump::ResetTrigger(bool updateLastCompletionTime)
{
  for (auto &listener : _fistBumpListeners) {
    listener->ResetTrigger(updateLastCompletionTime);
  }
}


} // namespace Cozmo
} // namespace Anki
