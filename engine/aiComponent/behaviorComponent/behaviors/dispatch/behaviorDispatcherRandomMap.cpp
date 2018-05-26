/**
 * File: BehaviorDispatcherRandomMap.cpp
 *
 * Author: Hamzah Khan
 * Created: 2018-05-24
 *
 * Description: A dispatcher that, upon initialization, maps a set of conditions to a set of behaviors (with replacement). 
 * Upon activation by a condition, the corresponding behavior will be set. This action occurs even if the behavior does 
 * not want to be active.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRandomMap.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "util/random/randomVectorSampler.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kBehaviorsKey = "behaviors";
const char* kConditionsKey = "conditions";

// used to ignore weighting in randomVectorSampler
const float kDefaultWeight = 1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandomMap::InstanceConfig::InstanceConfig()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandomMap::DynamicVariables::DynamicVariables()
{
  shouldEndAfterBehavior = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandomMap::BehaviorDispatcherRandomMap(const Json::Value& config)
 : BaseClass(config, false)
{
  const Json::Value& behaviorArray = config[kBehaviorsKey];
  const Json::Value& conditionArray = config[kConditionsKey];

  // check that this data is valid
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherRandomMap.BehaviorsNotSpecified",
                 "No Behaviors key found");
  DEV_ASSERT_MSG(!conditionArray.isNull(),
                 "BehaviorDispatcherRandomMap.ConditionsNotSpecified",
                 "No Conditions key found");

  if(!conditionArray.isNull() && !behaviorArray.isNull()) {

    for(const auto& behaviorDefinitionGroup: behaviorArray) {      
      const std::string& behaviorIDStr = JsonTools::ParseString(behaviorDefinitionGroup,
                                                                "behavior",
                                                                "BehaviorDispatcherRandom.BehaviorGroup.NoBehaviorID");
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr);      
    }

    // save each desired condition to the instance
    for(const auto& conditionDefinitionGroup: conditionArray) {
      _iConfig.conditions.push_back(
        BEIConditionFactory::CreateBEICondition(conditionDefinitionGroup, GetDebugLabel() )
      );
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandomMap::InitDispatcher() {
  // initialize random number generator tool
  Util::RandomVectorSampler<size_t> behaviorIndices;
  size_t behaviorNdx = 0;

  for(size_t i = 0; i < IBehaviorDispatcher::GetAllPossibleDispatches().size(); ++i) {
    behaviorIndices.PushBack(behaviorNdx, kDefaultWeight);
    ++behaviorNdx;                                  
  }

  for(const auto& condition: _iConfig.conditions) {
    condition->Init(GetBEI());
    condition->SetActive(GetBEI(), true);
    // add behaviors to the mapping
    _iConfig.mapping.emplace_back(
      behaviorIndices.Sample(GetRNG())
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandomMap::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  expectedKeys.insert( kConditionsKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandomMap::BehaviorDispatcher_OnActivated()
{
  _dVars.shouldEndAfterBehavior = false;
  // _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandomMap::BehaviorDispatcher_OnDeactivated()
{
  // // if we are stopped while a behavior was active, put it on cooldown
  // if( _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() ) {
  //   _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
  //   _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  // }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherRandomMap::GetDesiredBehavior()
{
  // DEV_ASSERT_MSG( _iConfig.cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
  //                 "BehaviorDispatcherRandom.CooldownSizeMismatch",
  //                 "have %zu cooldown infos, buts %zu possible dispatches",
  //                 _iConfig.cooldownInfo.size(),
  //                 IBehaviorDispatcher::GetAllPossibleDispatches().size());
  DEV_ASSERT_MSG( _iConfig.mapping.size() == _iConfig.conditions.size(),
                  "BehaviorDispatcherRandomMap.ConditionSizeMismatch",
                  "have %zu conditions, but %zu mappings",
                  _iConfig.mapping.size(),
                  _iConfig.conditions.size());

  // after the behavior we delegate to here finishes, we should cancel ourselves
  _dVars.shouldEndAfterBehavior = true;
  
  // iterate over all conditions, set cond_idx to the active one
  size_t cond_idx = 0;
  for( auto& condition : _iConfig.conditions) {
    if (condition->AreConditionsMet(GetBEI())) {
      auto behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(cond_idx);

      if (behavior->WantsToBeActivated()) {
        return behavior;
      }
    }
    ++cond_idx;
  }

  _dVars.shouldEndAfterBehavior = false;
  return ICozmoBehaviorPtr{};
  
  // for( size_t idx = 0; idx < numBehaviors; ++idx ) {
  //   const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

  //   if( behavior->IsActivated() || behavior->WantsToBeActivated() ) {

  //     PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.ConsiderBehavior",
  //                    "%s: considering '%s' with weight %f",
  //                    GetDebugLabel().c_str(),
  //                    behavior->GetDebugLabel().c_str(),
  //                    _iConfig.weights[idx]);
  //   }
  //   else {
  //     PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.IgnoreBehavior",
  //                    "%s: skipping '%s'",
  //                    GetDebugLabel().c_str(),
  //                    behavior->GetDebugLabel().c_str());
  //   }
  // }

  // if( !availableBehaviorIdxs.Empty() ) {
  //   size_t idx = availableBehaviorIdxs.Sample(GetRNG());
  //   _dVars.lastDesiredBehaviorIdx = idx;
  //   return IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);
  // }
  // else {
    
  //   // no behaviors were available, maybe it was because of cooldown? Go through again and find the one
  //   // closest to being off cooldown and return that
  //   float minCooldownRemaining = std::numeric_limits<float>::max();
  //   ICozmoBehaviorPtr bestBehavior;

  //   for( size_t idx = 0; idx < numBehaviors; ++idx ) {
  //     const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

  //     if( ( behavior->IsActivated() ||
  //           behavior->WantsToBeActivated() ) ) {
  //       auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
  //       const float remCooldown = cooldownInfo.GetRemainingCooldownTime_s();
  //       if( remCooldown < minCooldownRemaining ) {
  //         minCooldownRemaining = remCooldown;
  //         bestBehavior = behavior;
  //         _dVars.lastDesiredBehaviorIdx = idx;
  //       }
  //     }
  //   }

  //   if( bestBehavior != nullptr ) {
  //     PRINT_CH_INFO("Behaviors",
  //                   "BehaviorDispatcherRandom.AllOnCooldown",
  //                   "%s: all behaviors on cooldown, so selecting '%s' which is closest to being off cooldown (%f secs)",
  //                   GetDebugLabel().c_str(),
  //                   bestBehavior->GetDebugLabel().c_str(),
  //                   minCooldownRemaining);
  //   }

  //   return bestBehavior;
  // }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandomMap::DispatcherUpdate()
{

  if( _dVars.shouldEndAfterBehavior &&
      !IsControlDelegated() &&
      IsActivated() )
  {
    CancelSelf();
  }
}


} // namespace Cozmo
} // namespace Anki