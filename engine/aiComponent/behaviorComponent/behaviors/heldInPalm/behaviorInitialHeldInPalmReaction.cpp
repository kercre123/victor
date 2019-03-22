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
const char* const kAnimSelectorBehaviorName = "InitialHeldInPalmReactionInternal";
const BehaviorID kJoltBehaviorID = BEHAVIOR_ID(ReactToJoltInPalm);
const BehaviorID kPalmTiltBehaviorID = BEHAVIOR_ID(LookingNervousInPalmAnim);

const char* const kEmotionEventOnSuccessfulAnim = "InitialHeldInPalmReaction";
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::InstanceConfig::InstanceConfig()
{
  joltInPalmReaction = nullptr;
  palmTiltReaction = nullptr;
  animSelectorBehavior = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::DynamicVariables::DynamicVariables()
{
  animWasInterrupted = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::BehaviorInitialHeldInPalmReaction(const Json::Value& config)
 : ICozmoBehavior(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInitialHeldInPalmReaction::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  const std::string debugStr = "BehaviorInitialHeldInPalmReaction.Constructor.BehaviorNotFound";
  auto loadBehavior = [&BC, &debugStr](ICozmoBehaviorPtr& behavior, const BehaviorID& id){
    behavior = BC.FindBehaviorByID(id);
    ANKI_VERIFY( behavior != nullptr, debugStr.c_str(),
                 "Behavior %s could not be found",
                 BehaviorTypesWrapper::BehaviorIDToString(id));
  };
  
  loadBehavior(_iConfig.joltInPalmReaction, kJoltBehaviorID);
  loadBehavior(_iConfig.palmTiltReaction, kPalmTiltBehaviorID);
  
  auto loadAnonymousBehavior = [this, &debugStr](ICozmoBehaviorPtr& behavior, const std::string& behaviorName){
    behavior = FindBehavior(behaviorName);
    ANKI_VERIFY( behavior != nullptr, debugStr.c_str(),
                "Anonymous behavior %s could not be found",
                behaviorName.c_str());
  };
  
  loadAnonymousBehavior(_iConfig.animSelectorBehavior, kAnimSelectorBehaviorName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInitialHeldInPalmReaction::~BehaviorInitialHeldInPalmReaction()
{
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
  delegates.insert( _iConfig.palmTiltReaction.get() );
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
    
    LOG_DEBUG("BehaviorInitialHeldInPalmReaction.OnBehaviorActivated",
              "Initial get-in animation was %s this activation",
              _dVars.animWasInterrupted ? "interrupted" : "NOT interrupted");
  };
  DelegateIfInControl(_iConfig.animSelectorBehavior.get(), callback);
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
      } else if ( !_iConfig.palmTiltReaction->IsActivated() &&
                  _iConfig.palmTiltReaction->WantsToBeActivated() ) {
        _dVars.animWasInterrupted = true;
        // Just let the HeldInPalmResponses dispatcher handle the disruption, it isn't super-likely
        // that the user will just stop tilting or rolling Vector too much by the next engine tick.
        CancelSelf();
      }
    }
  }
}

}
}
