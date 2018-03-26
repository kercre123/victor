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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriority::InstanceConfig::InstanceConfig()
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
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
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
