/**
 * File: behaviorReactToCliff.cpp
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"

#define ALWAYS_PLAY_REACT_TO_CLIFF 1

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const float kCliffBackupDist_mm = 60.0f;
static const float kCliffBackupSpeed_mmps = 100.0f;

static const std::set<BehaviorType> kBehaviorsToDisable = {BehaviorType::ReactToUnexpectedMovement,
                                                           BehaviorType::AcknowledgeObject,
                                                           BehaviorType::AcknowledgeFace,
                                                           BehaviorType::ReactToCubeMoved};

BehaviorReactToCliff::BehaviorReactToCliff(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToCliff");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({{
    EngineToGameTag::CliffEvent,
    EngineToGameTag::RobotStopped
  }});
}

bool BehaviorReactToCliff::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToCliff::InitInternalReactionary(Robot& robot)
{
  robot.GetMoodManager().TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());
  
  for(auto behavior : kBehaviorsToDisable)
  {
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), behavior, false);
  }

  switch( _state ) {
    case State::PlayingStopReaction: 
      TransitionToPlayingStopReaction(robot);
      break;

    case State::PlayingCliffReaction:
      _gotCliff = true;
      TransitionToPlayingCliffReaction(robot);
      break;

    default: {
      PRINT_NAMED_ERROR("BehaviorReactToCliff.Init.InvalidState",
                        "Init called with invalid state");
      return Result::RESULT_FAIL;
    }
  }
  
  return Result::RESULT_OK;
}

void BehaviorReactToCliff::TransitionToPlayingStopReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingStopReaction);

  // in case latency spiked between the Stop and Cliff message, add a small extra delay
  const float latencyDelay_s = 0.05f;
  const float minWaitTime_s = (1.0 / 1000.0 ) * CLIFF_EVENT_DELAY_MS + latencyDelay_s;


  // play the stop animation, but also wait at least the minimum time so we keep running 
  _gotCliff = false;
  StartActing(new CompoundActionParallel(robot, {
    new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToCliffDetectorStop),
        new WaitAction(robot, minWaitTime_s) }),
    &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}


void BehaviorReactToCliff::TransitionToPlayingCliffReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingCliffReaction);
  
  if( _gotCliff || ALWAYS_PLAY_REACT_TO_CLIFF) {
    Anki::Util::sEvent("robot.cliff_detected", {}, "");
    StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToCliff),
                &BehaviorReactToCliff::TransitionToBackingUp);
  }
  // else end the behavior now
}

void BehaviorReactToCliff::TransitionToBackingUp(Robot& robot)
{
  // if the animation doesn't drive us backwards enough, do it manually
  if( robot.IsCliffDetected() ) {
      StartActing(new DriveStraightAction(robot, -kCliffBackupDist_mm, kCliffBackupSpeed_mmps),
                  [this,&robot](){
                      SendFinishedReactToCliffMessage(robot);
                  });
  }
  else {
    SendFinishedReactToCliffMessage(robot);
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToCliff);
  }
}
  
void BehaviorReactToCliff::SendFinishedReactToCliffMessage(Robot& robot) {
  // Send message that we're done reacting
  robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCliffEventFinished()));
}

void BehaviorReactToCliff::StopInternalReactionary(Robot& robot)
{
  _state = State::PlayingStopReaction;
  
  for(auto behavior : kBehaviorsToDisable)
  {
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), behavior, true);
  }
}

bool BehaviorReactToCliff::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{  
  switch( event.GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      if( !IsRunning() && event.Get_CliffEvent().detected ) {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.CliffWithoutStop",
                            "Got a cliff event but stop isn't running, skipping straight to cliff react (bad latency?)");
        // this should only happen if latency gets bad because otherwise we should stay in the stop reaction
        _state = State::PlayingCliffReaction;
        return true;
      }
      break;
    }

    case EngineToGameTag::RobotStopped: {
      if( !IsRunning() ) {
        _state = State::PlayingStopReaction;
        return true;
      }
      break;
    }

    default:
      PRINT_NAMED_ERROR("BehaviorReactToCliff.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;

  }

  return false;
} 

void BehaviorReactToCliff::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      if( !_gotCliff && event.GetData().Get_CliffEvent().detected ) {
        PRINT_NAMED_DEBUG("BehaviorReactToCliff.GotCliff", "Got cliff event while running");
        _gotCliff = true;
      }
      break;
    }

    default:
      break;
  }
}

} // namespace Cozmo
} // namespace Anki
