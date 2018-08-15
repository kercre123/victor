/**
 * File: behaviorDispatcherQueue.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-30
 *
 * Description: Simple dispatch behavior which runs each behavior in a list in order, if it wants to be
 *              activated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherQueue.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "json/json.h"
#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Vector {
  
namespace {
const char* kBehaviorsKey = "behaviors";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherQueue::InstanceConfig::InstanceConfig()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherQueue::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherQueue::BehaviorDispatcherQueue(const Json::Value& config)
  : IBehaviorDispatcher(config, false)
{
  const Json::Value& behaviorArray = config[kBehaviorsKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherQueue.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorIDStr: behaviorArray) {
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr.asString());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherQueue::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherQueue::BehaviorDispatcher_OnActivated()
{
  _dVars.currIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherQueue::DispatcherUpdate()
{
  if( IsActivated() &&
      !IsControlDelegated() &&
      _dVars.currIdx >= GetAllPossibleDispatches().size() ) {
    // we're past the end of the queue and control isn't delegated, so stop ourselves
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherQueue::GetDesiredBehavior()
{
  // This should only be callable when the behavior isn't running anymore
  DEV_ASSERT( !IsControlDelegated(), "BehaviorDispatcherQueue.GetDesiredBehavior.ControlNotDelegated" );

  const auto& dispatches = GetAllPossibleDispatches();

  // iterate until we find a behavior that wants to be activated
  BOUNDED_WHILE( 1000, _dVars.currIdx < dispatches.size() ) {
    if( dispatches[_dVars.currIdx]->WantsToBeActivated() ) {
      PRINT_CH_INFO("Behaviors", "BehaviorDispatcherQueue.SelectBehavior",
                    "Selecting behavior %zu '%s'",
                    _dVars.currIdx,
                    dispatches[_dVars.currIdx]->GetDebugLabel().c_str());

      // return this behavior and increment for next time
      return dispatches[_dVars.currIdx++];
    }
    else {
      // try the next behavior
      PRINT_CH_INFO("Behaviors", "BehaviorDispatcherQueue.SkipBehavior",
                    "Skipping behavior %zu '%s' because it doesn't want to run",
                    _dVars.currIdx,
                    dispatches[_dVars.currIdx]->GetDebugLabel().c_str());

      _dVars.currIdx++;
    }
  }

  // if we get here, it means none of the behaviors wanted to be activated
  return ICozmoBehaviorPtr{};
}

}
}
