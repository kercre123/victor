/**
 * File: behaviorReactToRobotShaken.cpp
 *
 * Author: Matt Michini
 * Created: 2017-01-11
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToRobotShaken.h"

#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{  
// Accelerometer magnitude threshold corresponding to "no longer shaking"
const float kAccelMagnitudeShakingStoppedThreshold = 13000.f;
// Dizzy factor thresholds for playing the soft, medium, or hard reactions
const float kShakenDurationThresholdHard   = 5.0f;
const float kShakenDurationThresholdMedium = 2.5f;
  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersRobotShakenArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotFalling,                 true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           true},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersRobotShakenArray),
              "Reaction triggers duplicate or non-sequential");

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::BehaviorReactToRobotShaken(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToRobotShaken::InitInternal(Robot& robot)
{
  // Disable some IMU-related behaviors
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersRobotShakenArray);
  
  // Clear severe needs expression since eyes are being re-set
  if(robot.GetAIComponent().GetSevereNeedsComponent().HasSevereNeedExpression())
  {
    robot.GetAIComponent().GetSevereNeedsComponent().ClearSevereNeedExpression();
  }
  
  // Reset variables:
  _maxShakingAccelMag = 0.f;
  _shakingStartedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _shakenDuration_s = 0.f;
  _reactionPlayed = EReaction::None;
  
  // Start the animations:
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyShakeLoop, 0));
  
  // Kick off the state machine:
  _state = EState::Shaking;
  
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorReactToRobotShaken::UpdateInternal(Robot& robot)
{

  // Master state machine:
  switch(_state) {
    case EState::Shaking:
    {
      const float accMag = robot.GetHeadAccelMagnitudeFiltered();
      _maxShakingAccelMag = std::max(_maxShakingAccelMag, accMag);
      
      // Done shaking? Then transition to the next state.
      if (accMag < kAccelMagnitudeShakingStoppedThreshold) {
        // Now that shaking has ended, determine how long it lasted:
        const float now_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _shakenDuration_s = now_s - _shakingStartedTime_s;
        _state = EState::DoneShaking;
      }
      
      break;
    }
    case EState::DoneShaking:
    {
      StopActing(false);
      CompoundActionSequential* action = new CompoundActionSequential(robot,
                                                                      {new TriggerAnimationAction(robot, AnimationTrigger::DizzyShakeStop),
                                                                       new TriggerAnimationAction(robot, AnimationTrigger::DizzyStillPickedUp)});
      StartActing(action);
      
      _state = EState::WaitTilOnTreads;
      
      break;
    }
    case EState::WaitTilOnTreads:
    {
      // Wait until on treads or the animations that are playing finish (timeout).
      if (robot.GetOffTreadsState() == OffTreadsState::OnTreads) {
        _state = EState::ActDizzy;
      } else if (!IsActing()) {
        // The "DizzyStillPickedUp" reaction played to completion, so log that as the played reaction:
        _reactionPlayed = EReaction::StillPickedUp;
        _state = EState::Finished;
      }
      
      break;
    }
    case EState::ActDizzy:
    {
      StopActing(false);
      // Play appropriate reaction based on duration of shaking:
      if (_shakenDuration_s > kShakenDurationThresholdHard) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionHard));
        _reactionPlayed = EReaction::Hard;
        NeedActionCompleted(NeedsActionId::DizzyHard);
      } else if (_shakenDuration_s > kShakenDurationThresholdMedium) {

        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionMedium));
        _reactionPlayed = EReaction::Medium;
        NeedActionCompleted(NeedsActionId::DizzyMedium);
      } else {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionSoft));
        _reactionPlayed = EReaction::Soft;
        NeedActionCompleted(NeedsActionId::DizzySoft);
      }
      
      _state = EState::Finished;
      break;
    }
    case EState::Finished:
    {
      if (!IsActing()) {
        // Done
        BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotShaken);
        return Status::Complete;
      }
    }
    default:
      break;
  }
  
  return Status::Running;
}
  

void BehaviorReactToRobotShaken::StopInternal(Robot& robot)
{
  // Log some DAS stuff:
  const int shakenDuration_ms = std::round(_shakenDuration_s * 1000.f);
  const int maxShakenAccelMag = std::round(_maxShakingAccelMag);
  // DAS event string: "<shakenDuration_ms>:<maxShakenAccelMag>"
  const std::string& data = std::to_string(shakenDuration_ms) + ":" + std::to_string(maxShakenAccelMag);
  Anki::Util::sEvent("robot.dizzy_reaction",
                     {{DDATA, data.c_str()}},
                     EReactionToString(_reactionPlayed));
  
  // Log human-readable completion info:
  PRINT_NAMED_INFO("BehaviorReactToRobotShaken.DizzyReaction",
                   "shakenDuration = %.3fs, maxShakingAccelMag = %.1f, reactionPlayed = '%s'",
                   _shakenDuration_s,
                   _maxShakingAccelMag,
                   EReactionToString(_reactionPlayed));
}
  
  
const char* BehaviorReactToRobotShaken::EReactionToString(EReaction reaction) const
{
  switch(reaction) {
    case EReaction::None:
      return "None";
    case EReaction::Soft:
      return "Soft";
    case EReaction::Medium:
      return "Medium";
    case EReaction::Hard:
      return "Hard";
    case EReaction::StillPickedUp:
      return "StillPickedUp";
    default:
      return "";
  }
}
        
  
} // namespace Cozmo
} // namespace Anki
