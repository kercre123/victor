/**
 * File: BehaviorInitialHeldInPalmReaction.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-03-18
 *
 * Description: Behavior that selects the appropriate animation to play when Vector was recently placed in a user's palm
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/heldInPalm/behaviorInitialHeldInPalmReaction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/moodSystem/moodManager.h"
#include "util/logging/DAS.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
  
namespace {
const BehaviorID kJoltBehaviorID = BEHAVIOR_ID(ReactToJoltInPalm);

const char* const kAnimSelectorBehaviorName = "InitialHeldInPalmReactionInternal";
const char* const kBehaviorsThatInterruptInitialReaction = "interruptingBehaviors";
const char* const kEmotionEventOnSuccessfulAnim = "InitialHeldInPalmReaction";
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::InstanceConfig::InstanceConfig()
{
  joltInPalmReaction = nullptr;
  animSelectorBehavior = nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  std::vector<std::string> tmpNames;
  if (JsonTools::GetVectorOptional(config, kBehaviorsThatInterruptInitialReaction, tmpNames)) {
    for(const auto& name : tmpNames) {
      interruptingBehaviorNames.insert(name);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::DynamicVariables::DynamicVariables()
{
  animWasInterrupted = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::BehaviorInitialHeldInPalmReaction(const Json::Value& config)
 : ICozmoBehavior(config),
   _iConfig(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::InitBehavior()
{
  const std::string debugStr = "BehaviorInitialHeldInPalmReaction.Constructor.BehaviorNotFound";
  
  // Find and cache the jolt-in-palm reaction behavior
  _iConfig.joltInPalmReaction = GetBEI().GetBehaviorContainer().FindBehaviorByID(kJoltBehaviorID);
  ANKI_VERIFY( _iConfig.joltInPalmReaction != nullptr, debugStr.c_str(),
               "Behavior %s could not be found",
               BehaviorTypesWrapper::BehaviorIDToString(kJoltBehaviorID));
  
  // Cache behaviors that can interrupt this initial reaction
  for(const auto& name : _iConfig.interruptingBehaviorNames){
    const ICozmoBehaviorPtr behavior = FindBehavior(name);
    if (ANKI_VERIFY(behavior != nullptr, debugStr.c_str(),
                    "Interrupting behavior %s could not be found", name.c_str() ) ) {
      _iConfig.interruptingBehaviors.insert(behavior);
    }
  }
  
  // Cache the animation selector behavior that is always delegated to upon activation
  _iConfig.animSelectorBehavior = FindBehavior(kAnimSelectorBehaviorName);
  ANKI_VERIFY(_iConfig.animSelectorBehavior != nullptr, debugStr.c_str(),
              "Animation selector behavior %s could not be found",
              kAnimSelectorBehaviorName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::~BehaviorInitialHeldInPalmReaction()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::GetBehaviorJsonKeys(std::set<const char *> &expectedKeys) const
{
  expectedKeys.insert( kBehaviorsThatInterruptInitialReaction );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInitialHeldInPalmReaction::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.joltInPalmReaction.get() );
  delegates.insert( _iConfig.animSelectorBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // By default, we always delegate to the animation selector behavior, so
  // verify that it always wants to be activated.
  if( !ANKI_VERIFY(_iConfig.animSelectorBehavior->WantsToBeActivated(),
              "BehaviorInitialHeldInPalmReaction.AnimationBehaviorDWTBA",
              "Animation behavior %s does not want to be activated",
                   _iConfig.animSelectorBehavior->GetDebugLabel().c_str())) {
    CancelSelf();
  }
  
  auto callback = [this](){
    if ( !_dVars.animWasInterrupted && GetBEI().HasMoodManager() ) {
      GetBEI().GetMoodManager().TriggerEmotionEvent(kEmotionEventOnSuccessfulAnim);
    }
    
    DASMSG(behavior_initial_hip_reaction,"behavior.initial_hip_reaction",
           "The robot has reacted to being initially held in a user's palm");
    DASMSG_SET(i1, _dVars.animWasInterrupted, "Was initial get-in animation interrupted?");
    DASMSG_SEND();
  };
  DelegateIfInControl(_iConfig.animSelectorBehavior.get(), callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::OnBehaviorDeactivated()
{
  LOG_DEBUG("BehaviorInitialHeldInPalmReaction.OnBehaviorDeactivated",
           "Initial get-in animation was%s interrupted during this activation",
           _dVars.animWasInterrupted ? "" : " NOT");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::BehaviorUpdate()
{
  if( IsActivated() ) {
    // Interrupt the initial animation if either the jolt reaction or palm-tilt reaction want to
    // be activated. Record the fact that the animation was interrupted so that the callback
    // assigned to it in OnBehaviorActivated publishes a DAS message that records a interrupted
    // playthrough and does NOT trigger the EmotionEvent associated with this behavior.
    if ( !_dVars.animWasInterrupted ) {
      if ( !_iConfig.joltInPalmReaction->IsActivated() &&
           _iConfig.joltInPalmReaction->WantsToBeActivated() ) {
        _dVars.animWasInterrupted = true;
        // The jolt reaction activation conditions may not hold true on the next engine tick, so
        // cancel the current delegate here now and switch to the jolt reaction right away.
        CancelDelegates();
        DelegateIfInControl(_iConfig.joltInPalmReaction.get());
      } else if ( BehaviorShouldBeInterrupted() ) {
        _dVars.animWasInterrupted = true;
        // Just let the HeldInPalm dispatcher handle the disruption, it isn't super-likely that
        // the behavior that wants to interrupt and deactivate this initial reaction will not want
        // to activate on the next engine tick.
        CancelSelf();
      }
    }
  }
}
  
bool BehaviorInitialHeldInPalmReaction::BehaviorShouldBeInterrupted() const
{
  for(const auto& behavior: _iConfig.interruptingBehaviors){
    if(behavior->WantsToBeActivated()){
      return true;
    }
  }
  return false;
}

}
}
