/**
 * File: behaviorDispatcherStrictPriorityWithCooldown.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Simple dispatcher similar to strict priority, but also supports cooldowns on behaviors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherStrictPriorityWithCooldown.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kBehaviorsKey = "behaviors";
const char* kLinkScopeKey = "linkScope";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::InstanceConfig::InstanceConfig()
  : linkScope(false)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::DynamicVariables::DynamicVariables()
{
  lastDesiredBehaviorIdx = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config)
  : BaseClass(config)
{

  _iConfig.linkScope = config.get(kLinkScopeKey, false).asBool();

  const Json::Value& behaviorArray = config[kBehaviorsKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorDefinitionGroup: behaviorArray) {      
      const std::string& behaviorIDStr = JsonTools::ParseString(
        behaviorDefinitionGroup,
        "behavior",
        "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorGroup.NoBehaviorID");
      
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr);

      _iConfig.cooldownInfo.emplace_back( BehaviorCooldownInfo{behaviorDefinitionGroup} );
    }
  }

  // initialize last idx to invalid value
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  expectedKeys.insert( kLinkScopeKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::GetLinkedActivatableScopeBehaviors(
  std::set<IBehavior*>& delegates) const
{
  if( _iConfig.linkScope ) {
    // all delegates are also linked in scope
    BaseClass::GetAllDelegates(delegates);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatcherStrictPriorityWithCooldown::WantsToBeActivatedBehavior() const
{
  if( !_iConfig.linkScope ) {
    // not linking, use the default implementation
    return BaseClass::WantsToBeActivatedBehavior();
  }

  const size_t numBehaviors = _iConfig.cooldownInfo.size();

  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);

    if( !cooldownInfo.OnCooldown() &&
        behavior->WantsToBeActivated() ) {
      return true;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriorityWithCooldown::GetDesiredBehavior()
{
  DEV_ASSERT_MSG( _iConfig.cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherStrictPriorityWithCooldown.SizeMismatch",
                  "have %zu cooldown infos, buts %zu possible dispatches",
                  _iConfig.cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());

  const size_t numBehaviors = _iConfig.cooldownInfo.size();

  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);

    if( !cooldownInfo.OnCooldown() &&
        ( behavior->IsActivated() ||
          behavior->WantsToBeActivated() ) ) {
      _dVars.lastDesiredBehaviorIdx = idx;
      return behavior;
    }
  }

  return ICozmoBehaviorPtr{};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::DispatcherUpdate()
{
  if( IsActivated() &&
      _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() &&
      !IsControlDelegated() ) {
    // the last behavior must have stopped, so start its cooldown now
    PRINT_CH_INFO("Behaviors",
                  "BehaviorDispatcherStrictPriorityWithCooldown.LastBehaviorStopped.StartCooldown",
                  "Behavior idx %zu '%s' seems to have stopped, start cooldown",
                  _dVars.lastDesiredBehaviorIdx,
                  IBehaviorDispatcher::GetAllPossibleDispatches()[_dVars.lastDesiredBehaviorIdx]->GetDebugLabel().c_str());
    _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnActivated()
{
  // reset last dispatched behavior
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnDeactivated()
{
  // if we are stopped while a behavior was active, put it on cooldown
  if( _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() ) {
    _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }   
}

}
}
