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

#include "anki/cozmo/basestation/behaviors/reactionary/BehaviorReactToRobotShaken.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// define member constants:
const float BehaviorReactToRobotShaken::_kAccelMagnitudeShakingStoppedThreshold = 13000.f;
const float BehaviorReactToRobotShaken::_kMaxDizzynessAccelMagnitude = 30000.f;
const float BehaviorReactToRobotShaken::_kMaxDizzynessDuration_s = 4.f;
const float BehaviorReactToRobotShaken::_kDizzynessThresholdHard = 0.85f;
const float BehaviorReactToRobotShaken::_kDizzynessThresholdMedium = 0.65f;

  
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
  _dizzynessFactor = 0.f;
  _maxShakingAccelMag = 0.f;
  _shakingStartedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
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
        _shakingEndedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _state = EState::DoneShaking;
      }
      
      break;
    }
    case EState::DoneShaking:
    {
      StopActing();
      CompoundActionSequential* action = new CompoundActionSequential(robot,
                                                                      {new TriggerAnimationAction(robot, AnimationTrigger::DizzyShakeStop),
                                                                       new TriggerAnimationAction(robot, AnimationTrigger::DizzyStillPickedUp)});
      StartActing(action);
      
      // Now that shaking has ended, compute Cozmo's "dizzyness factor" based on how
      //  long and how hard he was shaken.
      const float shakenDuration_s = _shakingEndedTime_s - _shakingStartedTime_s;
      
      float timeDizzynessFactor = shakenDuration_s / _kMaxDizzynessDuration_s;
      timeDizzynessFactor = std::min(1.f, timeDizzynessFactor);
      
      float magnitudeDizzynessFactor = (_maxShakingAccelMag - _kAccelMagnitudeShakingStoppedThreshold) /
                                       (_kMaxDizzynessAccelMagnitude - _kAccelMagnitudeShakingStoppedThreshold);
      magnitudeDizzynessFactor = std::min(1.f, magnitudeDizzynessFactor);
      
      _dizzynessFactor = std::max(timeDizzynessFactor, magnitudeDizzynessFactor);

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
      StopActing();
      // Play appropriate reaction based on dizzyness level:
      if (_dizzynessFactor > _kDizzynessThresholdHard) {
        StartActing(new TriggerAnimationAction(robot, AnimationTrigger::DizzyReactionHard));
      } else if (_dizzynessFactor > _kDizzynessThresholdMedium) {
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
