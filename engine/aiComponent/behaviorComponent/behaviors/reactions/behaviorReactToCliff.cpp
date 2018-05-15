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
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::DynamicVariables::DynamicVariables()
{
  cliffDetectThresholdAtStart = 0;
  quitReaction = false;
  state = State::PlayingStopReaction;
  gotCliff = false;
  gotStop = false;
  detectedFlags = 0;
  shouldStopDueToCharger = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::BehaviorReactToCliff(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({{
    EngineToGameTag::CliffEvent,
    EngineToGameTag::RobotStopped,
    EngineToGameTag::ChargerEvent
  }});

  _iConfig.cliffDetectedCondition = BEIConditionFactory::CreateBEICondition(BEIConditionType::CliffDetected, GetDebugLabel());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::InitBehavior()
{
  _iConfig.cliffDetectedCondition->Init(GetBEI());
  _iConfig.cliffDetectedCondition->SetActive(GetBEI(), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCliff::WantsToBeActivatedBehavior() const
{
  return _iConfig.cliffDetectedCondition->AreConditionsMet(GetBEI()) || _dVars.gotStop;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorActivated()
{
  // reset dvars
  _dVars = DynamicVariables();
  
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());
  }

  switch( _dVars.state ) {
    case State::PlayingStopReaction:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();

      // Record cliff detection threshold before at start of stop
      _dVars.cliffDetectThresholdAtStart = robotInfo.GetCliffSensorComponent().GetCliffDetectThreshold(0);

      // Wait function for determining if the cliff is suspicious
      auto waitForStopLambda = [this, &robotInfo](Robot& robot) {
        if ( robotInfo.GetMoveComponent().AreWheelsMoving() ) {
          return false;
        }

        if (_dVars.cliffDetectThresholdAtStart != robotInfo.GetCliffSensorComponent().GetCliffDetectThreshold(0)) {
          // There was a change in the cliff detection threshold so assuming
          // it was a false cliff and aborting reaction
          PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.QuittingDueToSuspiciousCliff", "");
          _dVars.quitReaction = true;
        }
        return true;
      };

      // skip the "huh" animation if in severe energy or repair
      auto callbackFunc = &BehaviorReactToCliff::TransitionToPlayingStopReaction;

      WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(waitForStopLambda);
      DelegateIfInControl(waitForStopAction, callbackFunc);
      break;
    }
    case State::PlayingCliffReaction:
      _dVars.gotCliff = true;
      TransitionToPlayingCliffReaction();
      break;

    default: {
      PRINT_NAMED_ERROR("BehaviorReactToCliff.Init.InvalidState",
                        "Init called with invalid state");
    }
  }


}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingStopReaction()
{
  DEBUG_SET_STATE(PlayingStopReaction);

  if(_dVars.quitReaction) {
    SendFinishedReactToCliffMessage();
    return;
  }

  // in case latency spiked between the Stop and Cliff message, add a small extra delay
  const float latencyDelay_s = 0.05f;
  const float maxWaitTime_s = (1.0f / 1000.0f ) * CLIFF_EVENT_DELAY_MS + latencyDelay_s;

  auto action = new CompoundActionParallel();

  // Wait for the cliff event before jumping to cliff reaction
  auto waitForCliffLambda = [this](Robot& robot) {
    return _dVars.gotCliff;
  };
  action->AddAction(new WaitForLambdaAction(waitForCliffLambda, maxWaitTime_s), true);
  DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingCliffReaction()
{
  DEBUG_SET_STATE(PlayingCliffReaction);

  if(ShouldStreamline()){
    TransitionToBackingUp();
  }
  else if( _dVars.gotCliff || ALWAYS_PLAY_REACT_TO_CLIFF) {
    Anki::Util::sInfo("robot.cliff_detected", {}, "");


    AnimationTrigger reactionAnim = AnimationTrigger::ReactToCliff;
    auto action = GetCliffPreReactAction(_dVars.detectedFlags);

    action->AddAction(new TriggerLiftSafeAnimationAction(reactionAnim));

    DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToBackingUp);
  }
  // else end the behavior now
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToBackingUp()
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  // if the animation doesn't drive us backwards enough, do it manually
  if( robotInfo.GetCliffSensorComponent().IsCliffDetected() ) {
      DelegateIfInControl(new DriveStraightAction(-kCliffBackupDist_mm, kCliffBackupSpeed_mmps),
                  [this](){
                      SendFinishedReactToCliffMessage();
                  });
  }
  else {
    SendFinishedReactToCliffMessage();
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToCliff);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::SendFinishedReactToCliffMessage() {
  // Send message that we're done reacting
  //robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCliffEventFinished()));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorDeactivated()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_dVars.shouldStopDueToCharger){
    _dVars.shouldStopDueToCharger = false;
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::HandleWhileInScopeButNotActivated(const EngineToGameEvent& event)
{
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      const auto detectedFlags = event.GetData().Get_CliffEvent().detectedFlags;
      if((detectedFlags != 0) && !_dVars.quitReaction) {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.CliffWithoutStop",
                            "Got a cliff event but stop isn't running, skipping straight to cliff react (bad latency?)");
        // this should only happen if latency gets bad because otherwise we should stay in the stop reaction
        _dVars.detectedFlags = detectedFlags;
        _dVars.gotCliff = true;
        _dVars.state = State::PlayingCliffReaction;
      }
      break;
    }

    case EngineToGameTag::RobotStopped: {
      _dVars.quitReaction = false;
      _dVars.gotStop = true;
      _dVars.state = State::PlayingStopReaction;
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
void BehaviorReactToCliff::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      const auto detectedFlags = event.GetData().Get_CliffEvent().detectedFlags;
      if( !_dVars.gotCliff && (detectedFlags != 0) ) {
        PRINT_NAMED_DEBUG("BehaviorReactToCliff.GotCliff", "Got cliff event while running");
        _dVars.gotCliff = true;
        _dVars.detectedFlags = detectedFlags;
      }
      break;
    }
    case EngineToGameTag::ChargerEvent:
    {
      // This isn't a real cliff, cozmo should stop reacting and let the drive off
      // charger action be selected
      if(event.GetData().Get_ChargerEvent().onCharger){
        _dVars.shouldStopDueToCharger = true;
      }
      break;
    }
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompoundActionSequential* BehaviorReactToCliff::GetCliffPreReactAction(uint8_t cliffDetectedFlags)
{
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));

  auto action = new CompoundActionSequential();

  float amountToTurn_deg = 0.f;
  float amountToDrive_mm = 0.f;
  bool turnThenDrive = true;

  // TODO: These actions should most likely be replaced by animations.
  switch (cliffDetectedFlags) {
    case (FL | FR):
      // Hit cliff straight-on. Play stop reaction and move on
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffDetectorStop));
      break;
    case FL:
      // Play stop reaction animation and turn CCW a bit
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffDetectorStop));
      amountToTurn_deg = 15.f;
      break;
    case FR:
      // Play stop reaction animation and turn CW a bit
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffDetectorStop));
      amountToTurn_deg = -15.f;
      break;
    case BL:
      // Drive forward and turn CCW to face the cliff
      amountToDrive_mm = 35.f;
      amountToTurn_deg = 135.f;
      turnThenDrive = false;
      break;
    case BR:
      // Drive forward and turn CW to face the cliff
      amountToDrive_mm = 35.f;
      amountToTurn_deg = -135.f;
      turnThenDrive = false;
      break;
    case (FL | BL):
      // Left side hanging off edge. Try to turn back onto the surface.
      amountToTurn_deg = 90.f;
      amountToDrive_mm = -30.f;
      break;
    case (FR | BR):
      // Right side hanging off edge. Try to turn back onto the surface.
      amountToTurn_deg = -90.f;
      amountToDrive_mm = -30.f;
      break;
    case (BL | BR):
      // Hit cliff straight-on driving backwards. Flip around to face the cliff.
      amountToDrive_mm = 35.f;
      amountToTurn_deg = 180.f;
      turnThenDrive = false;
      break;
    default:
      // In the default case, just play the stop reaction and move on.
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffDetectorStop));
      break;
  }

  auto turnAction = new TurnInPlaceAction(DEG_TO_RAD(amountToTurn_deg), false);
  turnAction->SetAccel(MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2);
  turnAction->SetMaxSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);

  auto driveAction = new DriveStraightAction(amountToDrive_mm, MAX_WHEEL_SPEED_MMPS, false);
  driveAction->SetAccel(MAX_WHEEL_ACCEL_MMPS2);
  driveAction->SetDecel(MAX_WHEEL_ACCEL_MMPS2);

  if (turnThenDrive) {
    if (amountToTurn_deg != 0.f) {
      action->AddAction(turnAction);
    }
    if (amountToDrive_mm != 0.f) {
      action->AddAction(driveAction);
    }
  } else {
    if (amountToDrive_mm != 0.f) {
      action->AddAction(driveAction);
    }
    if (amountToTurn_deg != 0.f) {
      action->AddAction(turnAction);
    }
  }

  return action;
}

} // namespace Cozmo
} // namespace Anki
