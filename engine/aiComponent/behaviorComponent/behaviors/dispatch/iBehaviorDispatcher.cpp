/**
 * File: iBehaviorDispatcher.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: common interface for dispatch-type behaviors. These are behaviors that only dispatch to other
 *              behaviors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kInterruptBehaviorKey = "interruptActiveBehavior";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorDispatcher::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorDispatcher::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorDispatcher::IBehaviorDispatcher(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _iConfig.shouldInterruptActiveBehavior = JsonTools::ParseBool(config,
                                                        kInterruptBehaviorKey,
                                                        "IBehaviorDispatcher.ShouldInterrupt.ConfigError");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorDispatcher::IBehaviorDispatcher(const Json::Value& config, bool shouldInterruptActiveBehavior)
  : ICozmoBehavior(config)
{
  _iConfig.shouldInterruptActiveBehavior = shouldInterruptActiveBehavior;

  // the config should either _not_ set the value at all, or it should be consistent with what the base class
  // passes in
  ANKI_VERIFY(config.get(kInterruptBehaviorKey, shouldInterruptActiveBehavior).asBool() == shouldInterruptActiveBehavior,
              "IBehaviorDispatcher.ConfigMismatch",
              "Behavior '%s' specified a '%s' value of %s, but behaviors of this type will ignore this flag",
              GetDebugLabel().c_str(),
              kInterruptBehaviorKey,
              config.get(kInterruptBehaviorKey, false).asBool() ? "true" : "false");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kInterruptBehaviorKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::InitBehavior()
{
  for( const auto& behaviorStr : _iConfig.behaviorStrs ) {
    // first check anonymous behaviors
    ICozmoBehaviorPtr behavior = FindAnonymousBehaviorByName(behaviorStr);
    if( nullptr == behavior ) {
      // no match, try behavior IDs
      const BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString(behaviorStr);
      behavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(behaviorID);
      
      DEV_ASSERT_MSG(behavior != nullptr,
                     "IBehaviorDispatcher.InitBehavior.FailedToFindBehavior",
                     "Behavior not found: %s",
                     behaviorStr.c_str());
    }
    if(behavior != nullptr){
      _iConfig.behaviors.push_back(behavior);
    }
  }

  // don't need the strings anymore, so clear them to release memory
  _iConfig.behaviorStrs.clear();
  
  InitDispatcher();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehaviorDispatcher::WantsToBeActivatedBehavior() const {

  // TODO:(bn) only wants to be activated if one of it's sub-behaviors does?
  // problem: they might not be in scope yet
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const 
{
  // set up return variables
  bool subBehaviorCarryingObject = false;
  bool subBehaviorOffTreads = false;
  bool subBehaviorOnCharger = false;
  
  // check all sub behavior values
  for( const auto& behaviorPtr : _iConfig.behaviors ) {
    BehaviorOperationModifiers subModifiers;
    behaviorPtr->GetBehaviorOperationModifiers(subModifiers);
    subBehaviorCarryingObject |= subModifiers.wantsToBeActivatedWhenCarryingObject;
    subBehaviorOffTreads      |= subModifiers.wantsToBeActivatedWhenOffTreads;
    subBehaviorOnCharger      |= subModifiers.wantsToBeActivatedWhenOnCharger;
  }

  // assign return variables
  modifiers.wantsToBeActivatedWhenCarryingObject = subBehaviorCarryingObject;
  modifiers.wantsToBeActivatedWhenOffTreads = subBehaviorOffTreads;
  modifiers.wantsToBeActivatedWhenOnCharger = subBehaviorOnCharger;

  // dispatchers always delegate to a behavior if they are meant to keep running
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& behaviorPtr : _iConfig.behaviors ) {
    delegates.insert( static_cast<IBehavior*>(behaviorPtr.get()) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  BehaviorDispatcher_OnActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::OnBehaviorDeactivated()
{
  BehaviorDispatcher_OnDeactivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehaviorDispatcher::CanBeGentlyInterruptedNow() const
{
  // it's a good time to interrupt if we aren't running a behavior, or if the behavior we're running says it's OK
  if( !IsControlDelegated() ) {
    return true;
  }
  else if( GetBEI().HasDelegationComponent() ) {
    auto& delegationComponent = GetBEI().GetDelegationComponent();

    const IBehavior* behaviorDelegatedTo = delegationComponent.GetBehaviorDelegatedTo(this);

    // TODO:(bn) work around this ugly cast by caching a version in ICozmoBehavior during the Delegate calls?
    DEV_ASSERT(dynamic_cast<const ICozmoBehavior*>(behaviorDelegatedTo),
               "IBehaviorDispatcher.DelegatedBehaviorNotACozmoBehavior");
    
    const ICozmoBehavior* delegate = static_cast<const ICozmoBehavior*>(behaviorDelegatedTo);

    return delegate != nullptr && delegate->CanBeGentlyInterruptedNow();
  }
  else {
    // no delegation component, so I guess it's an OK time?
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::BehaviorUpdate()
{
  DispatcherUpdate();
  if(!IsActivated()){
    return;
  }

  if( ! ANKI_VERIFY( GetBEI().HasDelegationComponent(),
                     "IBehaviorDispatcher.BehaviorUpdate.NoDelegationComponent",
                     "Behavior should have a delegation component while running") ) {
    CancelSelf();
    return;
  }

  // only choose a new behavior if we should interrupt the active behavior, or if no behavior is active
  if( ! IsControlDelegated() ||
      _iConfig.shouldInterruptActiveBehavior ) {
  
    auto& delegationComponent = GetBEI().GetDelegationComponent();
  
    const IBehavior* currBehavior = delegationComponent.GetBehaviorDelegatedTo(this);
    
    ICozmoBehaviorPtr desiredBehavior = GetDesiredBehavior();

    if( desiredBehavior != nullptr &&
        static_cast<IBehavior*>( desiredBehavior.get() ) != currBehavior ) {

      const bool delegated = DelegateNow(desiredBehavior.get());
      DEV_ASSERT_MSG(delegated, "IBehaviorDispatcher.BehaviorUpdate.DelegateFailed",
                     "Failed to delegate to behavior '%s'",
                     desiredBehavior->GetDebugLabel().c_str());
    }
  }
}

}
}
