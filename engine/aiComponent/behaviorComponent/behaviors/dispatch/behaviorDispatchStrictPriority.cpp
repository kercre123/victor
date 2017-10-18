/**
 * File: behaviorDispatchStrictPriority.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-13
 *
 * Description: Simple behavior which takes a json-defined list of other behaviors and dispatches to the first
 *              on in that list that wants to be activated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatchStrictPriority.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatchStrictPriority::BehaviorDispatchStrictPriority(const Json::Value& config)
  : ICozmoBehavior(config)
{
  const Json::Value& behaviorArray = config["behaviors"];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatchStrictPriority.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorIDStr: behaviorArray) {
      const BehaviorID behaviorID = BehaviorIDFromString(behaviorIDStr.asString());
      _behaviorIds.push_back( behaviorID );
    }
  }

  _shouldInterruptActiveBehavior = JsonTools::ParseBool(config,
                                                        "interruptActiveBehavior",
                                                        "BehaviorDispatchStrictPriority.ShouldInterrupt.ConfigError");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchStrictPriority::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  for( const auto& behaviorID : _behaviorIds ) {      
    ICozmoBehaviorPtr behavior =  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(behaviorID);
    DEV_ASSERT_MSG(behavior != nullptr,
                   "BehaviorDispatchStrictPriority.InitBehavior.FailedToFindBehavior",
                   "Behavior not found: %s",
                   BehaviorIDToString(behaviorID));
    if(behavior != nullptr){
      _behaviors.push_back(behavior);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatchStrictPriority::WantsToBeActivatedBehavior(
  BehaviorExternalInterface& behaviorExternalInterface) const {

  // TODO:(bn) only wants to be activated if one of it's sub-behaviors does?
  // problem: they might not be in scope yet
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatchStrictPriority::CarryingObjectHandledInternally() const
{
  for( const auto& behaviorPtr : _behaviors ) {
    if( behaviorPtr->CarryingObjectHandledInternally() ) {
      // if any of our behaviors can handle carrying an object, then so can we
      // NOTE: this might not be valid, because this behavior might not want to run
      return true;
    }
  }

  // if none of our sub-behaviors can handle carrying a cube, then neither can we
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatchStrictPriority::ShouldRunWhileOffTreads() const
{
  for( const auto& behaviorPtr : _behaviors ) {
    if( behaviorPtr->ShouldRunWhileOffTreads() ) {
      return true;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatchStrictPriority::ShouldRunWhileOnCharger() const
{
  for( const auto& behaviorPtr : _behaviors ) {
    if( behaviorPtr->ShouldRunWhileOnCharger() ) {
      return true;
    }
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchStrictPriority::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& behaviorPtr : _behaviors ) {
    delegates.insert( static_cast<IBehavior*>(behaviorPtr.get()) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDispatchStrictPriority::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorDispatchStrictPriority::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{

  if( ! ANKI_VERIFY( behaviorExternalInterface.HasDelegationComponent(),
                     "BehaviorDispatchStrictPriority.BehaviorUpdate.NoDelegationComponent",
                     "Behavior should have a delegation component while running") ) {
    return Status::Failure;
  }

  // only choose a new behavior if we should interrupt the active behavior, or if no behavior is active
  if( ! IsControlDelegated() ||
      _shouldInterruptActiveBehavior ) {
  
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
  
    const IBehavior* currBehavior = delegationComponent.GetBehaviorDelegatedTo(this);
    
    ICozmoBehaviorPtr desiredBehavior = nullptr;
  
    // Iterate through available behaviors, and use the first one that is activated or wants to be activated
    // since this is the highest priority behavior
    for(const auto& entry: _behaviors) {
      if(entry->IsActivated() || entry->WantsToBeActivated(behaviorExternalInterface)) {
        desiredBehavior = entry;
        break;
      }

      if( desiredBehavior != nullptr &&
          static_cast<IBehavior*>( desiredBehavior.get() ) != currBehavior ) {

        const bool delegated = DelegateNow(behaviorExternalInterface, desiredBehavior.get());
        DEV_ASSERT_MSG(delegated, "BehaviorDispatchStrictPriority.BehaviorUpdate.DelegateFailed",
                       "Failed to delegate to behavior '%s'",
                       desiredBehavior->GetIDStr().c_str());
      }
    }
  }

  return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatchStrictPriority::CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const
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
               "BehaviorDispatchStrictPriority.DelegatedBehaviorNotACozmoBehavior");
    
    const ICozmoBehavior* delegate = static_cast<const ICozmoBehavior*>(behaviorDelegatedTo);

    return delegate != nullptr && delegate->CanBeGentlyInterruptedNow(behaviorExternalInterface);
  }
  else {
    // no delegation component, so I guess it's an OK time?
    return true;
  }
}

}
}
