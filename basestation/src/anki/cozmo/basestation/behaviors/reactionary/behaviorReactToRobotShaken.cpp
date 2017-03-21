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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotShaken.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// define member constants:
const float BehaviorReactToRobotShaken::_kAccelMagnitudeShakingStoppedThreshold = 13000.f;
const float BehaviorReactToRobotShaken::_kShakenDurationThresholdHard   = 3.4f;
const float BehaviorReactToRobotShaken::_kShakenDurationThresholdMedium = 2.6f;

  
static const std::set<ReactionTrigger> kBehaviorsToDisable = {ReactionTrigger::CliffDetected,
                                                              ReactionTrigger::ReturnedToTreads,
                                                              ReactionTrigger::RobotOnBack,
                                                              ReactionTrigger::RobotOnFace,
                                                              ReactionTrigger::RobotOnSide,
                                                              ReactionTrigger::RobotPickedUp};
  
BehaviorReactToRobotShaken::BehaviorReactToRobotShaken(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToRobotShaken");
}


Result BehaviorReactToRobotShaken::InitInternal(Robot& robot)
{
  // Disable some IMU-related behaviors
  SmartDisableReactionTrigger(kBehaviorsToDisable);
  
  // Reset variables:
  _maxShakingAccelMag = 0.f;
  _shakingStartedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _shakenDuration_s = 0.f;
  
  
  // Start the animations:
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyShakeLoop, 0));
  
  // Kick off the state machine:
  _state = EState::Shaking;
  
  return Result::RESULT_OK;
}

  
IBehavior::Status BehaviorReactToRobotShaken::UpdateInternal(Robot& robot)
{

  // Master state machine:
  switch(_state) {
    case EState::Shaking:
    {
      const float accMag = robot.GetHeadAccelMagnitudeFiltered();
      _maxShakingAccelMag = std::max(_maxShakingAccelMag, accMag);
      
      // Done shaking? Then transition to the next state.
      if (accMag < _kAccelMagnitudeShakingStoppedThreshold) {
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
        _state = EState::Finished;
      }
      
      break;
    }
    case EState::ActDizzy:
    {
      StopActing(false);
      // Play appropriate reaction based on duration of shaking:
      if (_shakenDuration_s > _kShakenDurationThresholdHard) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionHard));
      } else if (_shakenDuration_s > _kShakenDurationThresholdMedium) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionMedium));
      } else {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionSoft));
      }
      
      _state = EState::Finished;
      break;
    }
    case EState::Finished:
    {
      if (!IsActing()) {
        // Bail out:
        BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotShaken);
        return Status::Complete;
      }
    }
    default:
      break;
  }
  
  return Status::Running;
}
  
  
} // namespace Cozmo
} // namespace Anki
