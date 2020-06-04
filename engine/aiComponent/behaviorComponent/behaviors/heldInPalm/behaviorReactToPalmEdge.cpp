/**
 * File: behaviorReactToPalmEdge.cpp
 *
 * Author: Guillermo Bautista
 * Created: 03/25/2019
 *
 * Description: Behavior for immediately responding to a robot driving both front cliff sensors
 *              or both rear cliff sensors over an edge when the robot is held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/heldInPalm/behaviorReactToPalmEdge.h"

#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/moodSystem/moodManager.h"

#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/proxMessages.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#include <limits>

#define CONSOLE_GROUP "Behavior.ReactToPalmEdge"
#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Vector {

namespace {
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  const uint8_t kFrontCliffSensors = FL | FR;
  const uint8_t kBackCliffSensors = BL | BR;
  
  // Configuration keys for recovery backup maneuver
  const char* kCliffBackupDistKey = "cliffBackupDistance_mm";
  const char* kCliffBackupSpeedKey = "cliffBackupSpeed_mmps";
  const char* kAnimOnActionFailureKey = "animOnTurnFailure";
  
  // Max rotational speed when turning to face a palm edge
  const f32 kMaxTurnSpeed_radps = DEG_TO_RAD(40.f);
  
  // If the behavior encounters this many failures with the initial animation/action
  // while activated, then just give up and go to ForceStuckOnPalmEdge.
  CONSOLE_VAR(uint32_t, kMaxNumInitialReactAttemptsBeforeGivingUp, CONSOLE_GROUP, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCliffBackupDistKey,
    kCliffBackupSpeedKey,
    kAnimOnActionFailureKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPalmEdge::InstanceConfig::InstanceConfig()
{
  askForHelpBehavior = nullptr;
  joltInPalmReaction = nullptr;
  
  animOnActionFailure = AnimationTrigger::Count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPalmEdge::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  cliffBackupDist_mm = JsonTools::ParseFloat(config, kCliffBackupDistKey, debugName);
  // Verify that backup distance is valid.
  ANKI_VERIFY(Util::IsFltGTZero(cliffBackupDist_mm), (debugName + ".InvalidCliffBackupDistance").c_str(),
              "Value should be greater than 0.0 (not %.2f).", cliffBackupDist_mm);
  
  cliffBackupSpeed_mmps = JsonTools::ParseFloat(config, kCliffBackupSpeedKey, debugName);
  // Verify that backup speed is valid.
  ANKI_VERIFY(Util::IsFltGTZero(cliffBackupSpeed_mmps), (debugName + ".InvalidCliffBackupSpeed").c_str(),
              "Value should be greater than 0.0 (not %.2f).", cliffBackupSpeed_mmps);
  
  std::string animOnTurnFailureName;
  if (JsonTools::GetValueOptional(config, kAnimOnActionFailureKey, animOnTurnFailureName)) {
    animOnActionFailure = AnimationTriggerFromString(animOnTurnFailureName);
    ANKI_VERIFY(animOnActionFailure != AnimationTrigger::Count,
                (debugName + ".InvalidAnimOnTurnFailure").c_str(),
                "%s is not a valid AnimationTrigger value", animOnTurnFailureName.c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPalmEdge::DynamicVariables::DynamicVariables()
{
  canBeInterruptedByJolt = true;
  hasTriggeredMoodEventForJolt = false;
  
  persistent.state = State::Inactive;
  persistent.numInitialReactAttempts = 0;
  persistent.cliffFlagsAtStart = 0;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPalmEdge::BehaviorReactToPalmEdge(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig = InstanceConfig(config, debugName);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.askForHelpBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ForceStuckOnPalmEdge));
  _iConfig.joltInPalmReaction = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToJoltInPalm));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPalmEdge::WantsToBeActivatedBehavior() const
{
  const auto& cliffComp = GetBEI().GetCliffSensorComponent();
  const uint8_t cliffFlags = cliffComp.GetCliffDetectedFlags();
  // Robot should react iff it is on a user's palm, and two cliff sensors on the same side
  // (front or back) have detected a cliff/edge.
  return GetBEI().GetHeldInPalmTracker().IsHeldInPalm() &&
         ( cliffFlags == kFrontCliffSensors || cliffFlags == kBackCliffSensors );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::OnBehaviorActivated()
{
  SET_STATE(Activating);
  // reset dvars
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("CliffReact");
  }
      
  auto& robotInfo = GetBEI().GetRobotInfo();

  // Wait function for determining if the cliff is suspicious
  auto waitForStopLambda = [this, &robotInfo](Robot& robot) {
    if ( robotInfo.GetMoveComponent().AreWheelsMoving() ) {
      return false;
    }

    const auto& cliffComp = robotInfo.GetCliffSensorComponent();
    const auto& cliffData = cliffComp.GetCliffDataRaw();
    const auto& cliffFlags = cliffComp.GetCliffDetectedFlags();

    LOG_INFO("BehaviorReactToPalmEdge.OnBehaviorActivated.CliffValsAfterStop",
             "%u %u %u %u (%x)", cliffData[0], cliffData[1], cliffData[2], cliffData[3], cliffFlags);
    
    // Track whether the cliff is in front or behind by caching the flags
    _dVars.persistent.cliffFlagsAtStart = cliffFlags;
    
    // Send DAS event each time the robot is asked to react to a new palm edge.
    {
      DASMSG(behavior_palm_edge_reaction, "behavior.palm_edge_reaction",
             "The robot reacted to an edge of a user's palm");
      DASMSG_SET(i1, _dVars.persistent.cliffFlagsAtStart, "Cliff detected flags");
      DASMSG_SEND();
    }

    return true;
  };

  WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(waitForStopLambda);
  DelegateIfInControl(waitForStopAction, &BehaviorReactToPalmEdge::TransitionToPlayingInitialReaction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::TransitionToStuckOnPalmEdge()
{
  SET_STATE(StuckOnPalmEdge);
  // We should ALWAYS be able to delegate to the AskForHelp behavior,
  // i.e. no activation conditions should block this delegation
  if (ANKI_VERIFY(_iConfig.askForHelpBehavior->WantsToBeActivated(),
                  "BehaviorReactToPalmEdge.TransitionToStuckOnPalmEdge.DoesNotWantToBeActivated",
                  "Behavior %s does not want to be activated!!",
                  _iConfig.askForHelpBehavior->GetDebugLabel().c_str()))
  {
    DelegateIfInControl(_iConfig.askForHelpBehavior.get());
  }
  // Behavior will cancel itself if it reaches this point
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::TransitionToPlayingInitialReaction()
{
  SET_STATE(PlayingInitialReaction);

  if(!WantsToBeActivated()) {
    const auto& cliffComponent = GetBEI().GetRobotInfo().GetCliffSensorComponent();
    LOG_INFO("BehaviorReactToPalmEdge.TransitionToPlayingInitialReaction",
             "%s does NOT want to be activated prior to initial reaction. Cliff flags: %x",
             GetDebugLabel().c_str(),
             cliffComponent.GetCliffDetectedFlags());
    return;
  }
  
  // Have we tried executing too many cliff reaction animations/actions for this
  // activation of the behavior? We must either be in a "cliffy" area, or somehow stuck in
  // some orientation that StuckOnPalmEdge is not detecting. Just go to StuckOnPalmEdge to be safe.
  if (_dVars.persistent.numInitialReactAttempts > kMaxNumInitialReactAttemptsBeforeGivingUp) {
    LOG_INFO("BehaviorReactToPalmEdge.TransitionToPlayingInitialReaction.TooManyAttempts", "");
    TransitionToStuckOnPalmEdge();
    return;
  }
  
  if (_dVars.persistent.cliffFlagsAtStart == 0) {
    // For some reason no cliffs are detected. Just leave the reaction.
    LOG_INFO("BehaviorReactToPalmEdge.TransitionToPlayingInitialReaction.NoCliffs", "");
    return;
  }
  
  // Get the pre-react action/animation to play
  auto action = GetInitialEdgeReactAction(_dVars.persistent.cliffFlagsAtStart);
  if (action == nullptr) {
    // No action was returned because the detected cliffs represent
    // a precarious situation so just delegate to StuckOnPalmEdge.
    LOG_INFO("BehaviorReactToPalmEdge.TransitionToPlayingInitialReaction.StuckOnPalmEdge",
             "Cliff Flags: %x", _dVars.persistent.cliffFlagsAtStart);
    TransitionToStuckOnPalmEdge();
  } else {
    ++_dVars.persistent.numInitialReactAttempts;
    _dVars.canBeInterruptedByJolt = false;
    DelegateIfInControl(action, [this](const ActionResult& res){
      _dVars.canBeInterruptedByJolt = true;
      if ( !WantsToBeActivated() ) {
        // The action succeeded in moving the robot enough to get it away from the edge of the
        // palm, such that at most only part of the edge is still detected by the cliff sensors.
        TransitionToFaceAndBackAwayFromEdge();
      } else {
        // Cliff reaction failed for some reason, something might be preventing the robot from
        // moving away from the edge, possibly because the treads are slipping on the user's palm
        // and therefore the robot is stuck on the edge of the palm.
        if (_iConfig.animOnActionFailure != AnimationTrigger::Count) {
          // If the user provided an animation to play when this happens, play the animation and
          // then retry the initial reaction.
          auto animOnFail = new TriggerLiftSafeAnimationAction(_iConfig.animOnActionFailure);
          
          // Lock the tracks, in case the selected animation causes some tread movement that might
          // cause the robot to fall off the edge of the user's palm.
          const u8 tracksToLock = Util::EnumToUnderlying(AnimTrackFlag::BODY_TRACK);
          animOnFail->SetTracksToLock(tracksToLock);
          
          // Afterwards, retry the initial reaction.
          DelegateIfInControl(animOnFail, &BehaviorReactToPalmEdge::TransitionToPlayingInitialReaction);
        } else {
          // If no animation was specified by the user, simply retry the initial reaction.
          TransitionToPlayingInitialReaction();
        }
      }
    });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::TransitionToRecoveryBackup()
{
  SET_STATE(RecoveryBackup);
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();

  // if the animation doesn't drive us backwards enough, do it manually
  if( cliffComp.IsCliffDetected() ) {
    
    // Determine whether to backup or move forward based on triggered sensor
    f32 direction = 1.f;
    if (cliffComp.IsCliffDetected(CliffSensor::CLIFF_FL) ||
        cliffComp.IsCliffDetected(CliffSensor::CLIFF_FR) ) {
      direction = -1.f;
    }

    LOG_INFO("BehaviorReactToPalmEdge.TransitionToRecoveryBackup.DoingExtraRecoveryMotion", "");
    IActionRunner* backupAction = new DriveStraightAction(direction * _iConfig.cliffBackupDist_mm,
                                                          _iConfig.cliffBackupSpeed_mmps);
    BehaviorSimpleCallback callback = [this](void)->void{
      LOG_INFO("BehaviorReactToPalmEdge.TransitionToRecoveryBackup.ExtraRecoveryMotionComplete", "");
      const auto& cliffComponent = GetBEI().GetRobotInfo().GetCliffSensorComponent();
      if ( WantsToBeActivated() ) {
        LOG_INFO("BehaviorReactToPalmEdge.TransitionToRecoveryBackup.StillStuckOnPalmEdge", "%x",
                 cliffComponent.GetCliffDetectedFlags());
        TransitionToStuckOnPalmEdge();
      }
    };
    DelegateIfInControl(backupAction, callback);
  } else {
    LOG_INFO("BehaviorReactToPalmEdge.TransitionToRecoveryBackup.NoCliffs", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::OnBehaviorDeactivated()
{
  SET_STATE(Inactive);
  // reset dvars
  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.askForHelpBehavior.get());
  delegates.insert(_iConfig.joltInPalmReaction.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  if ( _iConfig.joltInPalmReaction->WantsToBeActivated() ) {
    if ( _dVars.canBeInterruptedByJolt ) {
      TransitionToPlayingJoltReaction();
      return;
    } else if ( !_iConfig.joltInPalmReaction->IsActivated() && !_dVars.hasTriggeredMoodEventForJolt ){
      GetBEI().GetMoodManager().TriggerEmotionEvent("ReactToJoltInPalm");
      _dVars.hasTriggeredMoodEventForJolt = true;
    }
  } else {
    _dVars.hasTriggeredMoodEventForJolt = false;
  }
  
  const auto latestCliffFlags = GetBEI().GetCliffSensorComponent().GetCliffDetectedFlags();
  if ( WantsToBeActivated() &&
       _dVars.persistent.state != State::Activating &&
       latestCliffFlags != _dVars.persistent.cliffFlagsAtStart) {
    // One of the animations or actions we delegated to must have moved us over a different edge
    // edge of the palm when trying to back away from the current edge, we should re-start the reaction.
    CancelDelegates(false);
    OnBehaviorActivated();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  IActionRunner* BehaviorReactToPalmEdge::GetInitialEdgeReactAction(const uint8_t cliffDetectedFlags)
{
  IActionRunner* action = nullptr;

  LOG_INFO("ReactToPalmEdge.GetInitialEdgeReactAction.CliffsDetected", "0x%x", cliffDetectedFlags);

  // Play reaction animation based on triggered sensor(s)
  // Possibly supplement with "dramatic" reaction which involves
  // turning towards the cliff and backing up in a scared/shocked fashion.
  AnimationTrigger anim;
  switch (cliffDetectedFlags) {
    case (kFrontCliffSensors):
      // Hit cliff straight-on. Play stop reaction and move on
      anim = AnimationTrigger::ReactToCliffFront;
      break;
    case (kBackCliffSensors):
      // Hit cliff straight-on driving backwards. Flip around to face the cliff.
      anim = AnimationTrigger::ReactToCliffBack;
      break;
    default:
      // This is some scary configuration that we probably shouldn't move from.
      LOG_ERROR("ReactToPalmEdge.GetInitialEdgeReactAction.InvalidCliffFlags",
                "0x%x", cliffDetectedFlags);
      return nullptr;
  }
  
  action = new TriggerLiftSafeAnimationAction(anim);
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BehaviorReactToPalmEdge::TransitionToFaceAndBackAwayFromEdge()
{
  SET_STATE(FaceAndBackAwayFromEdge);
  IActionRunner* action = nullptr;

  // determine if we need to face the cliff before playing the backup action
  if( _dVars.persistent.cliffFlagsAtStart == kBackCliffSensors ) {
    auto compoundAction = new CompoundActionSequential();

    // Turn 180 degrees and animate simultaneously in order to face the cliff behind us.
    // The direction of the turn (left or right) is chosen at random.
    const float bodyTurnAngle = GetBEI().GetRNG().RandBool() ? M_PI : -1.0f * M_PI;
    AnimationTrigger anim = (bodyTurnAngle < 0.f) ?
      AnimationTrigger::ReactToCliffTurnRight180 : AnimationTrigger::ReactToCliffTurnLeft180;

    // combine a procedural turn with an animation
    auto waitAction = new WaitAction(0.25f); // constant time-delay prior to turn: reads better for animation
    auto turnAction = new TurnInPlaceAction(bodyTurnAngle, false);
    turnAction->SetMaxSpeed(kMaxTurnSpeed_radps);
    
    auto animAction = new TriggerLiftSafeAnimationAction(anim);

    auto waitTurn = new CompoundActionSequential();
    waitTurn->AddAction(waitAction);
    waitTurn->AddAction(turnAction);
    
    auto waitTurnAndAnimate = new CompoundActionParallel();
    waitTurnAndAnimate->AddAction(waitTurn);
    waitTurnAndAnimate->AddAction(animAction);
    
    compoundAction->AddAction(waitTurnAndAnimate);
    
    // Wait function for determining if the cliff is suspicious
    auto flagAsNonInterruptibleLambda = [this](Robot& robot) {
      _dVars.canBeInterruptedByJolt = false;
      return true;
    };
    
    compoundAction->AddAction(new WaitForLambdaAction(flagAsNonInterruptibleLambda));
    compoundAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::HeldOnPalmLookingNervous));
    action = compoundAction;
  } else {
    _dVars.canBeInterruptedByJolt = false;
    action = new TriggerLiftSafeAnimationAction(AnimationTrigger::HeldOnPalmLookingNervous);
  }
  
  DelegateIfInControl(action, [this](const ActionResult& res){
    _dVars.canBeInterruptedByJolt = true;
    if ( res != ActionResult::SUCCESS && _iConfig.animOnActionFailure != AnimationTrigger::Count ) {
      // If the action failed, play the animation selected/provided by the configuration
      auto animOnFail = new TriggerLiftSafeAnimationAction(_iConfig.animOnActionFailure);
      
      // Lock the tracks, in case the selected animation causes some tread movement that might cause
      // the robot to fall off the edge of the user's palm.
      const u8 tracksToLock = Util::EnumToUnderlying(AnimTrackFlag::BODY_TRACK);
      animOnFail->SetTracksToLock(tracksToLock);
      
      // Then execute a recovery backup action if the robot is still detecting cliffs at this point.
      DelegateIfInControl(animOnFail, &BehaviorReactToPalmEdge::TransitionToRecoveryBackup);
    } else {
      // Otherwise, simply proceed to check if we're still detecting cliffs, and activate a
      // recovery backup action if this is the case.
      TransitionToRecoveryBackup();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::TransitionToPlayingJoltReaction()
{
  SET_STATE(PlayingJoltReaction);
  
  const bool allowCallbacksToRun = false;
  CancelDelegates(allowCallbacksToRun);
  
  _dVars.canBeInterruptedByJolt = false;
  DelegateIfInControl(_iConfig.joltInPalmReaction.get(), [this](){
    _dVars.canBeInterruptedByJolt = true;
    if ( WantsToBeActivated() ) {
      TransitionToPlayingInitialReaction();
    }
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPalmEdge::SetState_internal(BehaviorReactToPalmEdge::State state,
                                                const std::string& stateName)
{
  _dVars.persistent.state = state;
  SetDebugStateName(stateName);
}

} // namespace Vector
} // namespace Anki
