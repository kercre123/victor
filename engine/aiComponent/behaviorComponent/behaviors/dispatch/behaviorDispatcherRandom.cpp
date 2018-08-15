/**
 * File: behaviorDispatcherRandom.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Dispatcher which runs behaviors randomly based on weights and cooldowns. Once the delegated-to behavior
 *              ends, this behavior will cancel itself (this should probably be configurable - see VIC-4836)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRandom.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "util/random/randomVectorSampler.h"

namespace Anki {
namespace Vector {
  
namespace {
const char* kBehaviorsKey = "behaviors";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandom::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandom::DynamicVariables::DynamicVariables()
{
  shouldEndAfterBehavior = false;
  lastDesiredBehaviorIdx = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandom::BehaviorDispatcherRandom(const Json::Value& config)
  : BaseClass(config, false)
{
  const Json::Value& behaviorArray = config[kBehaviorsKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorDefinitionGroup: behaviorArray) {      
      const std::string& behaviorIDStr = JsonTools::ParseString(behaviorDefinitionGroup,
                                                                "behavior",
                                                                "BehaviorDispatcherRandom.BehaviorGroup.NoBehaviorID");
      
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr);

      _iConfig.cooldownInfo.emplace_back( BehaviorCooldownInfo{behaviorDefinitionGroup} );

      const float weight = JsonTools::ParseFloat(behaviorDefinitionGroup,
                                                 "weight",
                                                 "BehaviorDispatcherRandom.BehaviorGroup.NoWeight");
      _iConfig.weights.push_back(weight);
                                                 
    }
  }
  
  // initialize last idx to invalid value
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::BehaviorDispatcher_OnActivated()
{
  _dVars.shouldEndAfterBehavior = false;
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::BehaviorDispatcher_OnDeactivated()
{
  // if we are stopped while a behavior was active, put it on cooldown
  if( _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() ) {
    _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherRandom::GetDesiredBehavior()
{
  DEV_ASSERT_MSG( _iConfig.cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherRandom.CooldownSizeMismatch",
                  "have %zu cooldown infos, buts %zu possible dispatches",
                  _iConfig.cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());
  DEV_ASSERT_MSG( _iConfig.weights.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherRandom.WeightSizeMismatch",
                  "have %zu weights, buts %zu possible dispatches",
                  _iConfig.cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());

  // after the behavior we delegate to here finishes, we should cancel ourselves
  _dVars.shouldEndAfterBehavior = true;
  
  Util::RandomVectorSampler<size_t> availableBehaviorIdxs;

  const size_t numBehaviors = _iConfig.cooldownInfo.size();
  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

    if( !cooldownInfo.OnCooldown() &&
        ( behavior->IsActivated() ||
          behavior->WantsToBeActivated() ) ) {
      availableBehaviorIdxs.PushBack(idx, _iConfig.weights[idx]);

      PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.ConsiderBehavior",
                     "%s: considering '%s' with weight %f",
                     GetDebugLabel().c_str(),
                     behavior->GetDebugLabel().c_str(),
                     _iConfig.weights[idx]);
    }
    else {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.IgnoreBehavior",
                     "%s: skipping '%s'",
                     GetDebugLabel().c_str(),
                     behavior->GetDebugLabel().c_str());
    }
  }

  if( !availableBehaviorIdxs.Empty() ) {
    size_t idx = availableBehaviorIdxs.Sample(GetRNG());
    _dVars.lastDesiredBehaviorIdx = idx;
    return IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);
  }
  else {
    
    // no behaviors were available, maybe it was because of cooldown? Go through again and find the one
    // closest to being off cooldown and return that
    float minCooldownRemaining = std::numeric_limits<float>::max();
    ICozmoBehaviorPtr bestBehavior;

    for( size_t idx = 0; idx < numBehaviors; ++idx ) {
      const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

      if( ( behavior->IsActivated() ||
            behavior->WantsToBeActivated() ) ) {
        auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
        const float remCooldown = cooldownInfo.GetRemainingCooldownTime_s();
        if( remCooldown < minCooldownRemaining ) {
          minCooldownRemaining = remCooldown;
          bestBehavior = behavior;
          _dVars.lastDesiredBehaviorIdx = idx;
        }
      }
    }

    if( bestBehavior != nullptr ) {
      PRINT_CH_INFO("Behaviors",
                    "BehaviorDispatcherRandom.AllOnCooldown",
                    "%s: all behaviors on cooldown, so selecting '%s' which is closest to being off cooldown (%f secs)",
                    GetDebugLabel().c_str(),
                    bestBehavior->GetDebugLabel().c_str(),
                    minCooldownRemaining);
    }

    return bestBehavior;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::DispatcherUpdate()
{
  if( _dVars.shouldEndAfterBehavior &&
      ! IsControlDelegated() &&
      IsActivated() )
  {
    CancelSelf();
  }
  
  if( IsActivated()
      && (_dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size())
      && !IsControlDelegated() )
  {
    // the last behavior must have stopped, so start its cooldown now
    PRINT_CH_INFO("Behaviors",
                  "BehaviorDispatcherRandom.LastBehaviorStopped.StartCooldown",
                  "Behavior idx %zu '%s' seems to have stopped, start cooldown",
                  _dVars.lastDesiredBehaviorIdx,
                  IBehaviorDispatcher::GetAllPossibleDispatches()[_dVars.lastDesiredBehaviorIdx]->GetDebugLabel().c_str());
    _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }
}


} // namespace Vector
} // namespace Anki
