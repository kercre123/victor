/**
 * File: BehaviorReactToImpact.cpp
 *
 * Author: Matt Michini
 * Date:   10/9/2017
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToImpact.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
  constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersImpactArray = {
    {ReactionTrigger::CliffDetected,                true},
    {ReactionTrigger::CubeMoved,                    true},
    {ReactionTrigger::FacePositionUpdated,          true},
    {ReactionTrigger::FistBump,                     true},
    {ReactionTrigger::Frustration,                  true},
    {ReactionTrigger::Hiccup,                       true},
    {ReactionTrigger::MotorCalibration,             true},
    {ReactionTrigger::NoPreDockPoses,               true},
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
    {ReactionTrigger::Sparked,                      true},
    {ReactionTrigger::UnexpectedMovement,           true},
  };
  
  static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersImpactArray),
                "Reaction triggers duplicate or non-sequential");
  
  const f32 kTimeout_sec = 5.f;
  const f32 kImpactIntensityToTriggerAnim = 1000.f;
}

  
BehaviorReactToImpact::BehaviorReactToImpact(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  // subscribe to some E2G messages:
  SubscribeToTags({{
    EngineToGameTag::FallingStarted,
    EngineToGameTag::FallingStopped,
    EngineToGameTag::MotorCalibration
  }});
}

  
Result BehaviorReactToImpact::InitInternal(Robot& robot)
{
  // Disable some reactions
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersImpactArray);
  
  // Start a wait action that will end when we receive the proper sequence of messages, or
  // will cause the behavior to end if the timeout is reached without receiving the messages.
  StartActing(new WaitForLambdaAction(robot,
                                      [this](Robot& robot) { return _doneFalling && _doneMotorCalibration; },
                                      kTimeout_sec),
              &BehaviorReactToImpact::TransitionToPlayingAnim);
  
  return RESULT_OK;
}


void BehaviorReactToImpact::TransitionToPlayingAnim(Robot& robot)
{
  if (_playImpactAnim) {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToImpact));
  }
}


void BehaviorReactToImpact::ResetFlags()
{
  _doneMotorCalibration = _doneFalling = _playImpactAnim = false;
}


void BehaviorReactToImpact::StopInternal(Robot& robot)
{
  ResetFlags();
}
  
  
void BehaviorReactToImpact::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  // Handle messages:
  switch( event.GetData().GetTag() ) {
      
    case EngineToGameTag::FallingStarted:
    {
      // Reset the flags here since we'll be expecting the FallingStopped
      // and MotorCalibration messages after this one.
      ResetFlags();
      break;
    }
    case EngineToGameTag::FallingStopped:
    {
      _doneFalling = true;
      // If the impact intensity was high, then play an animation.
      const auto& msg = event.GetData().Get_FallingStopped();
      _playImpactAnim = (msg.impactIntensity > kImpactIntensityToTriggerAnim);
      break;
    }
    case EngineToGameTag::MotorCalibration:
    {
      if (robot.IsHeadCalibrated() && robot.IsLiftCalibrated()) {
        _doneMotorCalibration = true;
      }
      break;
    }
    default:
      DEV_ASSERT_MSG(false,
                     "BehaviorReactToImpact.HandleWhileRunning.UnhandledMessage",
                     "Received an unhandled E2G event: %s",
                     MessageEngineToGameTagToString(event.GetData().GetTag()));
      break;
  }
}
  

}
}
