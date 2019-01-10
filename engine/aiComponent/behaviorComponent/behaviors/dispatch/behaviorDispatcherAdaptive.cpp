/**
 * File: BehaviorDispatcherAdaptive.cpp
 *
 * Author: Andrew Stout
 * Created: 2019-01-08
 *
 * Description:
 * 2019 R&D Sprint project: Adaptive Behavior Coordinator.
 * Chooses delegates according to a state-action-value function which is updated through very simple reinforcement
 * learning.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherAdaptive.h"

#define LOG_CHANNEL "BehaviorDispatcherAdaptive"

namespace Anki {
namespace Vector {

namespace {
  const char* kActionSpaceKey = "actionSpace"; // [json config file name of the] array of the behaviors in the action space
  const char* kDefaultBehaviorKey = "defaultBehavior"; // default behavior must wantToBeActivated in all states, and must be interruptable
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::BehaviorDispatcherAdaptive(const Json::Value& config)
 : IBehaviorDispatcher(config)
{
  LOG_WARNING("BehaviorDispatcherAdaptive.Constructor.Started", "");
  // read in the action space
  const Json::Value& actionNames = config[kActionSpaceKey];
  DEV_ASSERT_MSG(!actionNames.isNull(),
      "BehaviorDispatcherAdaptive.Constructor.ActionSpaceNotSpecified", "No %s found", kActionSpaceKey);
  if(!actionNames.isNull()) {
    for(const auto& behaviorIDStr: actionNames) { // not currently supporting cooldowns, etc. Could add.
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr.asString());
      // need to maintain the actionSpace separately
      // (default behavior will be last entry of _iConfig.behaviorStrs, but feels risky to count on that.)
      _iConfig.actionSpace.push_back(behaviorIDStr.asString());
    }
  }
  // read in the default behavior
  const Json::Value& defaultBehaviorName = config[kDefaultBehaviorKey];
  DEV_ASSERT_MSG(!defaultBehaviorName.isNull(),
                 "BehaviorDispatcherAdaptive.Constructor.DefaultBehaviorNotSpecified", "No %s found", kDefaultBehaviorKey);
  if(!defaultBehaviorName.isNull()) {
    IBehaviorDispatcher::AddPossibleDispatch(defaultBehaviorName.asString());
    _iConfig.defaultBehaviorName = defaultBehaviorName.asString(); // TODO: do I actually need this?
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::~BehaviorDispatcherAdaptive()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::InitDispatcher()
{
  // TODO: sanity check: check action lists here
  LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckBehaviorNames",
      "behaviorNames:\n");
  for(auto& b: GetAllPossibleDispatches()){
    LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckBehaviorStrs", "\t%s\n", b->GetDebugStateName().c_str());
  }
  LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckDefaultBehaviorName",
      "_iConfig.defaultBehaviorName: %s", _iConfig.defaultBehaviorName.c_str());
  // get the default behavior (not just the name)
  _iConfig.defaultBehavior = FindBehavior(_iConfig.defaultBehaviorName);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kActionSpaceKey,
      kDefaultBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::BehaviorDispatcher_OnActivated()
{
  // reset _dVars? Nope, already done in parent class
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::BehaviorDispatcher_OnDeactivated()
{
  // TODO: any learning vars that need to be cleaned up?
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherAdaptive::GetDesiredBehavior()
{
  // here's where we choose a new behavior
  // check state
  // if delegates other than Default are available
  // evaluate state-action value for each available delegate
  // for now, call method to get state action value for each available action,
  // build the "row" of the table for this state on the fly.
  // It would probably be more computationally efficient to do this differently in a real implementation.
  // probabilistically select

  // can I do the learning update here, or should that be in DispatcherUpdate?

  return _iConfig.defaultBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::DispatcherUpdate()
{
  // if we've delegated to a behavior other than Default, let it run
  // otherwise...
  // if something other than Default just finished, do a learning update
}

/*
// getState
State BehaviorDispatcherAdaptive::EvaluateState(){
  // TODO: implement (also, define State)
}

// get StateActionValue for state, action
SAValue getStateActionValue(State s, Action a){
  // TODO: implement for real
  return 0.0;
}
*/

// callback (?)


}
}
