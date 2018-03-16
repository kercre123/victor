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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/components/cliffSensorComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"
#include "engine/robotStateHistory.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/proxMessages.h"

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
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
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
bool BehaviorReactToCliff::IsRunnableInternal(const Robot& robot) const
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
      _cliffDetectThresholdAtStart = robot.GetCliffSensorComponent().GetCliffDetectThreshold();
      
      // Wait function for determining if the cliff is suspicious
      auto waitForStopLambda = [this](Robot& robot) {
        if ( robot.GetMoveComponent().AreWheelsMoving() ) {
          return false;
        }
        
        if (_cliffDetectThresholdAtStart != robot.GetCliffSensorComponent().GetCliffDetectThreshold()) {
          // There was a change in the cliff detection threshold so assuming
          // it was a false cliff and aborting reaction
          PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.QuittingDueToSuspiciousCliff", "");
          _quitReaction = true;
        }
        return true;
      };
      
      // skip the "huh" animation if in severe energy or repair
      auto callbackFunc = &BehaviorReactToCliff::TransitionToPlayingStopReaction;
      NeedId expressedNeed = robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
      if((expressedNeed == NeedId::Energy) || (expressedNeed == NeedId::Repair)){
        callbackFunc = &BehaviorReactToCliff::TransitionToPlayingCliffReaction;
      }
      
      WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(robot, waitForStopLambda);
      StartActing(waitForStopAction, callbackFunc);
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
  const float maxWaitTime_s = (1.0f / 1000.0f ) * CLIFF_EVENT_DELAY_MS + latencyDelay_s;

  
  auto action = new CompoundActionParallel(robot);
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToCliffDetectorStop));
  
  // Wait for the cliff event before jumping to cliff reaction
  auto waitForCliffLambda = [this](Robot& robot) {
    return _gotCliff;
  };
  action->AddAction(new WaitForLambdaAction(robot, waitForCliffLambda, maxWaitTime_s), true);
  StartActing(action, &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingCliffReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingCliffReaction);
  
  if(ShouldStreamline()){
    TransitionToBackingUp(robot);
  }
  else if( _gotCliff || ALWAYS_PLAY_REACT_TO_CLIFF) {
    Anki::Util::sEvent("robot.cliff_detected", {}, "");
    
    
    AnimationTrigger reactionAnim = AnimationTrigger::ReactToCliff;
    
    // special animations for maintaining eye shape in severe need states
    const NeedId severeExpressedNeed = robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
    if(NeedId::Energy == severeExpressedNeed){
      reactionAnim = AnimationTrigger::NeedsSevereLowEnergyCliffReact;
    }else if(NeedId::Repair == severeExpressedNeed){
      reactionAnim = AnimationTrigger::NeedsSevereLowRepairCliffReact;
    }
    
    auto action = new TriggerLiftSafeAnimationAction(robot, reactionAnim);

    StartActing(action, &BehaviorReactToCliff::TransitionToBackingUp);
  }
  // else end the behavior now
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToBackingUp(Robot& robot)
{
  // if the animation doesn't drive us backwards enough, do it manually
  if( robot.GetCliffSensorComponent().IsCliffDetected() ) {
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
  _gotCliff = false;
  _detectedFlags = 0;
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
      const auto detectedFlags = event.GetData().Get_CliffEvent().detectedFlags;
      if((detectedFlags != 0) && !_quitReaction) {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.CliffWithoutStop",
                            "Got a cliff event but stop isn't running, skipping straight to cliff react (bad latency?)");
        // this should only happen if latency gets bad because otherwise we should stay in the stop reaction
        _detectedFlags = detectedFlags;
        _gotCliff = true;
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
      const auto detectedFlags = event.GetData().Get_CliffEvent().detectedFlags;
      if( !_gotCliff && (detectedFlags != 0) ) {
        PRINT_NAMED_DEBUG("BehaviorReactToCliff.GotCliff", "Got cliff event while running");
        _gotCliff = true;
        _detectedFlags = detectedFlags;
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
