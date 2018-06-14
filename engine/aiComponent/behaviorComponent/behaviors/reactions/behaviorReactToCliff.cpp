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

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/proxMessages.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/components/movementComponent.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include <limits>

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

namespace{
static const float kCliffBackupDist_mm = 60.0f;
static const float kCliffBackupSpeed_mmps = 100.0f;

// If the value of the cliff when it started stopping is 
// this much less than the value when it stopped, then
// the cliff is considered suspicious (i.e. something like
// carpet) and the reaction is aborted.
// In general you'd expect the start value to be _higher_
// that the stop value if it's true cliff, but we
// use some margin here to account for sensor noise.
static const u16   kSuspiciousCliffValDiff = 20;

// If this many RobotStopped messages are received
// while activated, then stop trying to do the cliff
// reaction because maybe it's backing us up too far.
static const int   kMaxNumRobotStopsBeforeSkippingReactionAnim = 3;

// If this many RobotStopped messages are received
// while activated, then just give up and go to StuckOnEdge.
// It's probably too dangerous to keep trying anything
static const int   kMaxNumRobotStopsBeforeGivingUp = 5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::InstanceConfig::InstanceConfig()
{
  stuckOnEdgeBehavior = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::DynamicVariables::DynamicVariables()
{
  quitReaction = false;
  state = State::PlayingStopReaction;
  gotCliff = false;
  gotStop = false;
  shouldStopDueToCharger = false;
  wantsToBeActivated = false;

  const auto kInitVal = std::numeric_limits<u16>::max();
  std::fill(persistent.cliffValsAtStart.begin(), persistent.cliffValsAtStart.end(), kInitVal);
  persistent.numStops = 0;
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
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.stuckOnEdgeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(StuckOnEdge));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCliff::WantsToBeActivatedBehavior() const
{
  return _dVars.gotStop || _dVars.gotCliff || _dVars.wantsToBeActivated;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorActivated()
{
  // reset dvars
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());
  }

  switch( _dVars.state ) {
    case State::PlayingStopReaction:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();

      // Wait function for determining if the cliff is suspicious
      auto waitForStopLambda = [this, &robotInfo](Robot& robot) {
        if ( robotInfo.GetMoveComponent().AreWheelsMoving() ) {
          return false;
        }

        const auto& cliffComp = robotInfo.GetCliffSensorComponent();
        const auto& cliffData = cliffComp.GetCliffDataRaw();

        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.CliffValsAtEnd", 
                      "%u %u %u %u (%x)", 
                      cliffData[0], cliffData[1], cliffData[2], cliffData[3], cliffComp.GetCliffDetectedFlags());

        for (int i=0; i<CliffSensorComponent::kNumCliffSensors; ++i) {  
          if (cliffComp.IsCliffDetected((CliffSensor)(i))) {
            if (_dVars.persistent.cliffValsAtStart[i] + kSuspiciousCliffValDiff < cliffData[i]) {
              // There was a sufficiently large increase in cliff value since cliff was 
              /// first detected so assuming it was a false cliff and aborting reaction
              PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.QuittingDueToSuspiciousCliff", 
                            "index: %d, startVal: %u, currVal: %u",
                            i, _dVars.persistent.cliffValsAtStart[i], cliffData[i]);
              _dVars.quitReaction = true;    
            }
          }
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
void BehaviorReactToCliff::TransitionToStuckOnEdge()
{
  ANKI_VERIFY(_iConfig.stuckOnEdgeBehavior->WantsToBeActivated(),
              "BehaviorReactToCliff.TransitionToStuckOnEdge.DoesNotWantToBeActivated", 
              "");
  DelegateIfInControl(_iConfig.stuckOnEdgeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingStopReaction()
{
  DEBUG_SET_STATE(PlayingStopReaction);

  if(_dVars.quitReaction) {
    return;
  }

  // in case latency spiked between the Stop and Cliff message
  const float maxWaitTime_s = (1.0f / 1000.0f ) * CLIFF_EVENT_DELAY_MS;

  // Wait for the cliff event before jumping to cliff reaction
  auto waitForCliffLambda = [this](Robot& robot) {
    return _dVars.gotCliff;
  };
  auto action = new WaitForLambdaAction(waitForCliffLambda, maxWaitTime_s);
  DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingCliffReaction()
{
  DEBUG_SET_STATE(PlayingCliffReaction);

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::ReactedToCliff);
  
  if(ShouldStreamline()){
    TransitionToBackingUp();
  } else {
    Anki::Util::sInfo("robot.cliff_detected", {}, "");

    auto cliffDetectedFlags = GetBEI().GetRobotInfo().GetCliffSensorComponent().GetCliffDetectedFlags();
    if (cliffDetectedFlags == 0) {
      // For some reason no cliffs are detected. Just leave the reaction.
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.NoCliffs", "");
      CancelSelf();
      return;
    }

    // Did we get too many RobotStopped messages for this
    // activation of the behavior? Must be in a very "cliffy" area.
    // Just go to StuckOnEdge to be safe.
    if (_dVars.persistent.numStops > kMaxNumRobotStopsBeforeGivingUp) {
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.TooManyRobotStops", "");
      TransitionToStuckOnEdge();
      return;
    }

    // Get the pre-react action/animation to play
    auto action = GetCliffPreReactAction(cliffDetectedFlags);
    if (action == nullptr) {
      // No action was returned because the detected cliffs represent 
      // a precarious situation so just delegate to StuckOnEdge.
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.StuckOnEdge", 
                       "%x", 
                       cliffDetectedFlags);
      TransitionToStuckOnEdge();
    } else {

      // Did we get too many RobotStopped messages for this
      // activation of the behavior? Try not playing this backup reaction.
      if (_dVars.persistent.numStops <= kMaxNumRobotStopsBeforeSkippingReactionAnim) {
        AnimationTrigger reactionAnim = AnimationTrigger::ReactToCliff;
        action->AddAction(new TriggerLiftSafeAnimationAction(reactionAnim));
      }

      DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToBackingUp);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToBackingUp()
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  // if the animation doesn't drive us backwards enough, do it manually
  if( robotInfo.GetCliffSensorComponent().IsCliffDetected() ) {
    
    // Determine whether to backup or move forward based on triggered sensor
    f32 direction = 1.f;
    if (robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FL) ||
        robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FR) ) {
      direction = -1.f;
    }

    PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.DoingExtraRecoveryMotion", "");
    DelegateIfInControl(new DriveStraightAction(direction * kCliffBackupDist_mm, kCliffBackupSpeed_mmps),
                  [this](){
                      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.ExtraRecoveryMotionComplete", "");

                      auto& cliffComponent = GetBEI().GetRobotInfo().GetCliffSensorComponent();
                      if ( cliffComponent.IsCliffDetected() ) {
                        PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.StillStuckOnEdge", 
                                         "%x", 
                                         cliffComponent.GetCliffDetectedFlags());
                        TransitionToStuckOnEdge();
                      }
                  });
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToCliff);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorDeactivated()
{
  // reset dvars
  _dVars = DynamicVariables();
}


void BehaviorReactToCliff::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.stuckOnEdgeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::BehaviorUpdate()
{
  if(!IsActivated()){
    // Set wantsToBeActivated to effectively give the activation conditions
    // an extra tick to be evaluated.
    _dVars.wantsToBeActivated = _dVars.gotStop || _dVars.gotCliff;
    _dVars.gotStop = false;
    _dVars.gotCliff = false;
    return;
  }

  // TODO: This exit on unexpected movement is probably good to have, 
  // but it appears the cliff reactions cause unexpected movement to 
  // trigger falsely, so commenting out until unexpected movement
  // is modified to have fewer false positives.
  //  
  // // Delegate to StuckOnEdge if unexpected motion detected while
  // // cliff is still detected since it means treads are spinning
  // const bool unexpectedMovement = GetBEI().GetMovementComponent().IsUnexpectedMovementDetected();
  // const bool cliffDetected = GetBEI().GetRobotInfo().GetCliffSensorComponent().IsCliffDetected();
  // if (unexpectedMovement && cliffDetected) {
  //   PRINT_NAMED_INFO("BehaviorReactToCliff.Update.StuckOnEdge", "");
  //   _iConfig.stuckOnEdgeBehavior->WantsToBeActivated();
  //   DelegateNow(_iConfig.stuckOnEdgeBehavior.get());
  // }

  // Cancel if picked up
  const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();
  if (isPickedUp) {
    PRINT_NAMED_INFO("BehaviorReactToCliff.Update.CancelDueToPickup", "");
    CancelSelf();
  }

  if(_dVars.shouldStopDueToCharger){
    _dVars.shouldStopDueToCharger = false;
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  const bool alreadyActivated = IsActivated();
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::CliffEvent: {
      const auto detectedFlags = event.GetData().Get_CliffEvent().detectedFlags;
      if(!alreadyActivated && (detectedFlags != 0) && !_dVars.quitReaction) {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.CliffWithoutStop",
                            "Got a cliff event but stop isn't running, skipping straight to cliff react (bad latency?)");
        // this should only happen if latency gets bad because otherwise we should stay in the stop reaction
        _dVars.gotCliff = true;
        _dVars.state = State::PlayingCliffReaction;
      }
      break;
    }

    case EngineToGameTag::RobotStopped: {
      if (event.GetData().Get_RobotStopped().reason != StopReason::CLIFF) {
        break;
      }
      
      _dVars.quitReaction = false;
      _dVars.gotStop = true;
      _dVars.state = State::PlayingStopReaction;
      ++_dVars.persistent.numStops;

      // Record triggered cliff sensor value(s) and compare to what they are when wheels
      // stop moving. If the values are higher, the cliff is suspicious and we should quit.
      auto& robotInfo = GetBEI().GetRobotInfo();
      const auto& cliffComp = robotInfo.GetCliffSensorComponent();
      const auto& cliffData = cliffComp.GetCliffDataRaw();
      std::copy(cliffData.begin(), cliffData.end(), _dVars.persistent.cliffValsAtStart.begin());
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.CliffValsAtStart", 
                    "%u %u %u %u (alreadyActivated: %d)", 
                    _dVars.persistent.cliffValsAtStart[0], 
                    _dVars.persistent.cliffValsAtStart[1],
                    _dVars.persistent.cliffValsAtStart[2],
                    _dVars.persistent.cliffValsAtStart[3],
                    alreadyActivated);

      if (alreadyActivated) {
        CancelDelegates(false);
        OnBehaviorActivated();
      }
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

  PRINT_NAMED_INFO("ReactToCliff.GetCliffPreReactAction.CliffsDetected", "0x%x", cliffDetectedFlags);

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
    case (BL | BR):
      // Hit cliff straight-on driving backwards. Flip around to face the cliff.
      amountToDrive_mm = 35.f;
      amountToTurn_deg = 180.f;
      turnThenDrive = false;
      break;
    default:
      // This is some scary configuration that we probably shouldn't move from.
      delete(action);
      return nullptr;
  }

  auto turnAction = new TurnInPlaceAction(DEG_TO_RAD(amountToTurn_deg), false);
  turnAction->SetAccel(MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2);
  turnAction->SetMaxSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);

  auto driveAction = new DriveStraightAction(amountToDrive_mm, MAX_SAFE_WHEEL_SPEED_MMPS, false);
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
