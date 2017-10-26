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

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriority::BehaviorDispatcherStrictPriority(const Json::Value& config)
  : IBehaviorDispatcher(config)
{
  const Json::Value& behaviorArray = config["behaviors"];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriority.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorIDStr: behaviorArray) {
      const BehaviorID behaviorID = BehaviorIDFromString(behaviorIDStr.asString());
      IBehaviorDispatcher::AddPossibleDispatch(behaviorID);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriority::GetDesiredBehavior(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  // Iterate through available behaviors, and use the first one that is activated or wants to be activated
  // since this is the highest priority behavior
  for(const auto& entry: GetAllPossibleDispatches()) {
    if(entry->IsActivated() || entry->WantsToBeActivated(behaviorExternalInterface)) {
      return entry;
    }
  }

  return ICozmoBehaviorPtr{};
}

}
}
