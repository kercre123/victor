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
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"

#define ALWAYS_PLAY_REACT_TO_CLIFF 1

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

namespace{
static const float kCliffBackupDist_mm = 60.0f;
static const float kCliffBackupSpeed_mmps = 100.0f;


constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersReactToCliffArray = {
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
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersReactToCliffArray),
              "Reaction triggers duplicate or non-sequential");
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::BehaviorReactToCliff(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _shouldStopDueToCharger(false)
{
  SubscribeToTags({{
    EngineToGameTag::CliffEvent,
    EngineToGameTag::RobotStopped,
    EngineToGameTag::ChargerEvent
  }});
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCliff::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToCliff::InitInternal(Robot& robot)
{
  robot.GetMoodManager().TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());  
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersReactToCliffArray);
  
  switch( _state ) {
    case State::PlayingStopReaction:
    {
      // Record cliff detection threshold before at start of stop
      _cliffDetectThresholdAtStart = robot.GetCliffDetectThreshold();
      
      // Wait function for determining if the cliff is suspicious
      auto waitForStopLambda = [this](Robot& robot) {
        if ( robot.GetMoveComponent().AreWheelsMoving() ) {
          return false;
        }
        
        if (_cliffDetectThresholdAtStart != robot.GetCliffDetectThreshold()) {
          // There was a change in the cliff detection threshold so assuming
          // it was a false cliff and aborting reaction
          PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.QuittingDueToSuspiciousCliff", "");
          _quitReaction = true;
        }
        return true;
      };
      
      WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(robot, waitForStopLambda);
      StartActing(waitForStopAction, &BehaviorReactToCliff::TransitionToPlayingStopReaction);
      break;
    }
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingStopReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingStopReaction);

  if(_quitReaction) {
    SendFinishedReactToCliffMessage(robot);
    return;
  }
  
  // in case latency spiked between the Stop and Cliff message, add a small extra delay
  const float latencyDelay_s = 0.05f;
  const float minWaitTime_s = (1.0f / 1000.0f ) * CLIFF_EVENT_DELAY_MS + latencyDelay_s;

  // play the stop animation, but also wait at least the minimum time so we keep running
  _gotCliff = false;
  StartActing(new CompoundActionParallel(robot, {
    new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToCliffDetectorStop),
    new WaitAction(robot, minWaitTime_s) }),
    &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingCliffReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingCliffReaction);
  
  if(_shouldStreamline){
    TransitionToBackingUp(robot);
  }
  else if( _gotCliff || ALWAYS_PLAY_REACT_TO_CLIFF) {
    Anki::Util::sEvent("robot.cliff_detected", {}, "");
    StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToCliff),
                &BehaviorReactToCliff::TransitionToBackingUp);
  }
  // else end the behavior now
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::SendFinishedReactToCliffMessage(Robot& robot) {
  // Send message that we're done reacting
  robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCliffEventFinished()));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::StopInternal(Robot& robot)
{
  _state = State::PlayingStopReaction;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorReactToCliff::UpdateInternal(Robot& robot)
{
  if(_shouldStopDueToCharger){
    _shouldStopDueToCharger = false;
    return Status::Complete;
  }
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{  
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      if(event.GetData().Get_CliffEvent().detected && !_quitReaction) {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.CliffWithoutStop",
                            "Got a cliff event but stop isn't running, skipping straight to cliff react (bad latency?)");
        // this should only happen if latency gets bad because otherwise we should stay in the stop reaction
        _state = State::PlayingCliffReaction;
      }
      break;
    }

    case EngineToGameTag::RobotStopped: {
      _quitReaction = false;
      _state = State::PlayingStopReaction;
      break;
    }
    case EngineToGameTag::ChargerEvent:
    {
      // This is fine, we don't care about this event when we're not running
      break;
    }

    default:
      PRINT_NAMED_ERROR("BehaviorReactToCliff.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;

  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
    case EngineToGameTag::ChargerEvent:
    {
      // This isn't a real cliff, cozmo should stop reacting and let the drive off
      // charger action be selected
      if(event.GetData().Get_ChargerEvent().onCharger){
        _shouldStopDueToCharger = true;
      }
      break;
    }
    default:
      break;
  }
}


} // namespace Cozmo
} // namespace Anki
