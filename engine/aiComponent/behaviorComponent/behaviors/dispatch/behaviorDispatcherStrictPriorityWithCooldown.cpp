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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config)
  : BaseClass(config)
{
  const Json::Value& behaviorArray = config["behaviors"];
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

      _cooldownInfo.emplace_back( BehaviorCooldownInfo{behaviorDefinitionGroup} );
    }
  }

  // initialize last idx to invalid value
  _lastDesiredBehaviorIdx = _cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriorityWithCooldown::GetDesiredBehavior()
{
  DEV_ASSERT_MSG( _cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherStrictPriorityWithCooldown.SizeMismatch",
                  "have %zu cooldown infos, buts %zu possible dispatches",
                  _cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());

  const size_t numBehaviors = _cooldownInfo.size();

  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);

    if( !cooldownInfo.OnCooldown() &&
        ( behavior->IsActivated() ||
          behavior->WantsToBeActivated() ) ) {
      _lastDesiredBehaviorIdx = idx;
      return behavior;
    }
  }

  return ICozmoBehaviorPtr{};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::DispatcherUpdate()
{
  if( IsActivated() &&
      _lastDesiredBehaviorIdx < _cooldownInfo.size() &&
      !IsControlDelegated() ) {
    // the last behavior must have stopped, so start its cooldown now
    PRINT_CH_INFO("Behaviors",
                  "BehaviorDispatcherStrictPriorityWithCooldown.LastBehaviorStopped.StartCooldown",
                  "Behavior idx %zu '%s' seems to have stopped, start cooldown",
                  _lastDesiredBehaviorIdx,
                  IBehaviorDispatcher::GetAllPossibleDispatches()[_lastDesiredBehaviorIdx]->GetDebugLabel().c_str());
    _cooldownInfo[ _lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _lastDesiredBehaviorIdx = _cooldownInfo.size();
  }  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnActivated()
{
  // reset last dispatched behavior
  _lastDesiredBehaviorIdx = _cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnDeactivated()
{
  // if we are stopped while a behavior was active, put it on cooldown
  if( _lastDesiredBehaviorIdx < _cooldownInfo.size() ) {
    _cooldownInfo[ _lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _lastDesiredBehaviorIdx = _cooldownInfo.size();
  }   
}

}
}
