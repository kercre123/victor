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
IBehaviorDispatcher::IBehaviorDispatcher(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _shouldInterruptActiveBehavior = JsonTools::ParseBool(config,
                                                        kInterruptBehaviorKey,
                                                        "IBehaviorDispatcher.ShouldInterrupt.ConfigError");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorDispatcher::IBehaviorDispatcher(const Json::Value& config, bool shouldInterruptActiveBehavior)
  : ICozmoBehavior(config)
{
  _shouldInterruptActiveBehavior = shouldInterruptActiveBehavior;

  // the config should either _not_ set the value at all, or it should be consistent with what the base class
  // passes in
  ANKI_VERIFY(config.get(kInterruptBehaviorKey, shouldInterruptActiveBehavior).asBool() == shouldInterruptActiveBehavior,
              "IBehaviorDispatcher.ConfigMismatch",
              "Behavior '%s' specified a '%s' value of %s, but behaviors of this type will ignore this flag",
              GetIDStr().c_str(),
              kInterruptBehaviorKey,
              config.get(kInterruptBehaviorKey, false).asBool() ? "true" : "false");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  for( const auto& behaviorID : _behaviorIds ) {      
    ICozmoBehaviorPtr behavior =  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(behaviorID);
    DEV_ASSERT_MSG(behavior != nullptr,
                   "IBehaviorDispatcher.InitBehavior.FailedToFindBehavior",
                   "Behavior not found: %s",
                   BehaviorTypesWrapper::BehaviorIDToString(behaviorID));
    if(behavior != nullptr){
      _behaviors.push_back(behavior);
    }
  }
  InitDispatcher(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehaviorDispatcher::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const {

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
  for( const auto& behaviorPtr : _behaviors ) {
    behaviorPtr->GetBehaviorOperationModifiers(modifiers);
    subBehaviorCarryingObject |= modifiers.wantsToBeActivatedWhenCarryingObject;
    subBehaviorOffTreads      |= modifiers.wantsToBeActivatedWhenOffTreads;
    subBehaviorOnCharger      |= modifiers.wantsToBeActivatedWhenOnCharger;
  }

  // assign return variables
  modifiers.wantsToBeActivatedWhenCarryingObject = subBehaviorCarryingObject;
  modifiers.wantsToBeActivatedWhenOffTreads = subBehaviorOffTreads;
  modifiers.wantsToBeActivatedWhenOnCharger = subBehaviorOnCharger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& behaviorPtr : _behaviors ) {
    delegates.insert( static_cast<IBehavior*>(behaviorPtr.get()) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  BehaviorDispatcher_OnActivated(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::OnCozmoBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  BehaviorDispatcher_OnDeactivated(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehaviorDispatcher::CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // it's a good time to interrupt if we aren't running a behavior, or if the behavior we're running says it's OK
  if( !IsControlDelegated() ) {
    return true;
  }
  else if( behaviorExternalInterface.HasDelegationComponent() ) {
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();

    const IBehavior* behaviorDelegatedTo = delegationComponent.GetBehaviorDelegatedTo(this);

    // TODO:(bn) work around this ugly cast by caching a version in ICozmoBehavior during the Delegate calls?
    DEV_ASSERT(dynamic_cast<const ICozmoBehavior*>(behaviorDelegatedTo),
               "IBehaviorDispatcher.DelegatedBehaviorNotACozmoBehavior");
    
    const ICozmoBehavior* delegate = static_cast<const ICozmoBehavior*>(behaviorDelegatedTo);

    return delegate != nullptr && delegate->CanBeGentlyInterruptedNow(behaviorExternalInterface);
  }
  else {
    // no delegation component, so I guess it's an OK time?
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehaviorDispatcher::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  DispatcherUpdate(behaviorExternalInterface);
  if(!IsActivated()){
    return;
  }

  if( ! ANKI_VERIFY( behaviorExternalInterface.HasDelegationComponent(),
                     "IBehaviorDispatcher.BehaviorUpdate.NoDelegationComponent",
                     "Behavior should have a delegation component while running") ) {
    CancelSelf();
    return;
  }

  // only choose a new behavior if we should interrupt the active behavior, or if no behavior is active
  if( ! IsControlDelegated() ||
      _shouldInterruptActiveBehavior ) {
  
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
  
    const IBehavior* currBehavior = delegationComponent.GetBehaviorDelegatedTo(this);
    
    ICozmoBehaviorPtr desiredBehavior = GetDesiredBehavior(behaviorExternalInterface);

    if( desiredBehavior != nullptr &&
        static_cast<IBehavior*>( desiredBehavior.get() ) != currBehavior ) {

      const bool delegated = DelegateNow(behaviorExternalInterface, desiredBehavior.get());
      DEV_ASSERT_MSG(delegated, "IBehaviorDispatcher.BehaviorUpdate.DelegateFailed",
                     "Failed to delegate to behavior '%s'",
                     desiredBehavior->GetIDStr().c_str());
    }
  }
}

}
}
