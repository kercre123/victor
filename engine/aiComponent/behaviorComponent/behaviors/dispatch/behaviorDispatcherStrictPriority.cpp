/**
 * File: behaviorDispatcherStrictPriority.cpp
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

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherStrictPriority.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kBehaviorsKey = "behaviors";
const char* kLinkScopeKey = "linkScope";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriority::InstanceConfig::InstanceConfig()
  : linkScope(false)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriority::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriority::BehaviorDispatcherStrictPriority(const Json::Value& config)
  : IBehaviorDispatcher(config)
{
  static_assert(std::is_base_of<BehaviorDispatcherStrictPriority::BaseClass, BehaviorDispatcherStrictPriority>::value,
              "BaseClass is wrong");

  _iConfig.linkScope = config.get(kLinkScopeKey, false).asBool();
  
  const Json::Value& behaviorArray = config[kBehaviorsKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriority.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorIDStr: behaviorArray) {
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr.asString());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriority::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  expectedKeys.insert( kLinkScopeKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriority::GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.linkScope ) {
    // all delegates are also linked in scope
    BaseClass::GetAllDelegates(delegates);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatcherStrictPriority::WantsToBeActivatedBehavior() const
{
  if( !_iConfig.linkScope ) {
    // not linking, use the default implementation
    return BaseClass::WantsToBeActivatedBehavior();
  }

  for(const auto& entry: GetAllPossibleDispatches()) {
    if(entry->WantsToBeActivated()) {
      return true;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriority::GetDesiredBehavior()
{
  // Iterate through available behaviors, and use the first one that is activated or wants to be activated
  // since this is the highest priority behavior
  for(const auto& entry: GetAllPossibleDispatches()) {
    if(entry->IsActivated() || entry->WantsToBeActivated()) {
      return entry;
    }
  }

  return ICozmoBehaviorPtr{};
}

} // namespace Cozmo
} // namespace Anki
