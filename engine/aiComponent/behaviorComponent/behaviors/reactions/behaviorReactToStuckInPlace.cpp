/**
 * File: BehaviorReactToStuckInPlace.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2018-11-07
 *
 * Description: Behavior for reacting to a situation where the robot has been trapped in a location due to external factors,
 * such as getting caught on a cable or wire, or having the lift snagged on a ledge.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToStuckInPlace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kFrequentActivationHistoryTrackerKey = "frequentActivationHistoryTracker";
  const char* kFailureActivationHistoryTrackerKey = "failureActivationHistoryTracker";
  const char* kRetreatDistanceKey = "retreatDistance_mm";
  const char* kRetreatSpeedKey = "retreatSpeed_mmps";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToStuckInPlace::InstanceConfig::InstanceConfig() :
  activationHistoryTracker( "ReactToStuckInPlaceActivationHistoryTracker" )
{
  askForHelpBehavior = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToStuckInPlace::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName) :
  activationHistoryTracker( "ReactToStuckInPlaceActivationHistoryTracker" )
{
  int numRepeatedActivationsAllowed;
  
  // Configure occurrence tracker handle to monitor frequent activations
  ANKI_VERIFY( RecentOccurrenceTracker::ParseConfig(config[kFrequentActivationHistoryTrackerKey],
                                                    numRepeatedActivationsAllowed,
                                                    frequentActivationCheckWindow_sec),
              (debugName + "InvalidTrackerConfig").c_str(),
              "Behavior specified invalid recent occurrence config for checking for frequent activations");
  frequentActivationHandle = activationHistoryTracker.GetHandle(numRepeatedActivationsAllowed, frequentActivationCheckWindow_sec);
  
  // Configure occurrence tracker handle to monitor when behavior has activated enough times to declare failure
  ANKI_VERIFY( RecentOccurrenceTracker::ParseConfig(config[kFailureActivationHistoryTrackerKey],
                                                    numRepeatedActivationsAllowed,
                                                    failureActivationCheckWindow_sec),
              (debugName + "InvalidTrackerConfig").c_str(),
              "Behavior specified invalid recent occurrence config for checking for excessive activations");
  failureActivationHandle = activationHistoryTracker.GetHandle(numRepeatedActivationsAllowed, failureActivationCheckWindow_sec);

  retreatDistance_mm = JsonTools::ParseFloat(config, kRetreatDistanceKey, debugName);
  if (!ANKI_VERIFY(Util::IsFltGTZero(retreatDistance_mm),
                   (debugName + ".NonPositiveDistance").c_str(),
                   "Retreat distance should always be positive (not %f). Making positive.",
                   retreatDistance_mm))
  {
    retreatDistance_mm *= -1.0;
  }
  retreatSpeed_mmps = JsonTools::ParseFloat(config, kRetreatSpeedKey, debugName);
  if (!ANKI_VERIFY(Util::IsFltGTZero(retreatSpeed_mmps),
                   (debugName + ".NonPositiveSpeed").c_str(),
                   "Retreat speed should always be positive (not %f). Making positive.",
                   retreatSpeed_mmps))
  {
    retreatSpeed_mmps *= -1.0;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToStuckInPlace::BehaviorReactToStuckInPlace(const Json::Value& config)
 : ICozmoBehavior(config),
   _iConfig(config, ("Behavior" + GetDebugLabel() + ".LoadConfig"))
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.askForHelpBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(AskForHelp));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToStuckInPlace::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.askForHelpBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kFrequentActivationHistoryTrackerKey,
    kFailureActivationHistoryTrackerKey,
    kRetreatDistanceKey,
    kRetreatSpeedKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::OnBehaviorActivated() 
{
  // Reset dynamic variables
  _dVars = DynamicVariables();
  
  // Update activation history
  _iConfig.activationHistoryTracker.AddOccurrence();
  
  // Depending on the number/frequency of activations of this behavior, delegate control to an appropriate action:
  
  // Scenario 1: Activated extremely frequently, even emergency point-turn maneuevers are failing to get robot unstuck,
  // delegate to a behavior that asks the user for help getting the robot un-stuck.
  if (ShouldAskForHelp()) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToUnexpectedMovement.OnBehaviorActivated.RepeatedlyActivated",
                  "Activated %zu times in the past %.1f seconds.",
                  _iConfig.activationHistoryTracker.GetCurrentSize(),
                  _iConfig.failureActivationCheckWindow_sec);

    if (ANKI_VERIFY(_iConfig.askForHelpBehavior->WantsToBeActivated(),
                    "BehaviorReactToStuckInPlace.DelegateToAskForHelp",
                    "Behavior %s does not want to activate! Canceling self.",
                    _iConfig.askForHelpBehavior->GetDebugLabel().c_str())) {
      // Reset the records of activation times prior to delegating to prevent retriggering this delegation if the
      // ReactToStuckInPlace is reactivated sometime in the near future.
      _iConfig.activationHistoryTracker.Reset();
      DelegateIfInControl(_iConfig.askForHelpBehavior.get());
    } else {
      CancelSelf();
    }
  } else if (ShouldExecuteEmergencyTurn()) {
    // Scenario 2: Activated somewhat frequently, perform fast fixed-angle point turn, then try to quickly move
    // forward or backward to get untangled from cord/wire that may be caught in treads.
    PRINT_CH_INFO("Behaviors", "BehaviorReactToUnexpectedMovement.OnBehaviorActivated.RepeatedlyActivated",
                  "Activated %zu times in the past %.1f seconds.",
                  _iConfig.activationHistoryTracker.GetCurrentSize(),
                  _iConfig.frequentActivationCheckWindow_sec);
    CompoundActionSequential* seq_action = new CompoundActionSequential({
      new TurnInPlaceAction(M_PI/3.f, false),
      new DriveStraightAction(2.f * _iConfig.retreatDistance_mm, 5.f * _iConfig.retreatSpeed_mmps)
    });
    DelegateIfInControl(seq_action);
  } else {
    // Scenario 3: No frequent activations detected, just raise the lift and back up slowly,
    // the lift may have snagged on something.
    CompoundActionSequential* seq_action = new CompoundActionSequential({
      new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::CARRY),
      new DriveStraightAction(-_iConfig.retreatDistance_mm, _iConfig.retreatSpeed_mmps)});
    DelegateIfInControl(seq_action);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToStuckInPlace::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
  }
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToStuckInPlace::ShouldExecuteEmergencyTurn() const
{
  return _iConfig.frequentActivationHandle->AreConditionsMet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToStuckInPlace::ShouldAskForHelp() const
{
  return _iConfig.failureActivationHandle->AreConditionsMet();
}

}
}
