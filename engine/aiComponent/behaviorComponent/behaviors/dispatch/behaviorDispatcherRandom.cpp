/**
 * File: behaviorDispatcherRandom.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Dispatcher which runs behaviors randomly based on weights and cooldowns
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRandom.h"

#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "util/random/randomVectorSampler.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRandom::BehaviorDispatcherRandom(const Json::Value& config)
  : BaseClass(config, false)
{
  const Json::Value& behaviorArray = config["behaviors"];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorDefinitionGroup: behaviorArray) {      
      const std::string& behaviorIDStr = JsonTools::ParseString(behaviorDefinitionGroup,
                                                                "behavior",
                                                                "BehaviorDispatcherRandom.BehaviorGroup.NoBehaviorID");
      
      const BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString(behaviorIDStr);
      IBehaviorDispatcher::AddPossibleDispatch(behaviorID);

      _cooldownInfo.emplace_back( BehaviorCooldownInfo{behaviorDefinitionGroup} );

      const float weight = JsonTools::ParseFloat(behaviorDefinitionGroup,
                                                 "weight",
                                                 "BehaviorDispatcherRandom.BehaviorGroup.NoWeight");
      _weights.push_back(weight);
                                                 
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::BehaviorDispatcher_OnActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _shouldEndAfterBehavior = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherRandom::GetDesiredBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT_MSG( _cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherRandom.CooldownSizeMismatch",
                  "have %zu cooldown infos, buts %zu possible dispatches",
                  _cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());
  DEV_ASSERT_MSG( _weights.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherRandom.WeightSizeMismatch",
                  "have %zu weights, buts %zu possible dispatches",
                  _cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());

  // after the behavior we delegate to here finishes, we should cancel ourselves
  _shouldEndAfterBehavior = true;
  
  Util::RandomVectorSampler<ICozmoBehaviorPtr> availableBehaviors;

  const size_t numBehaviors = _cooldownInfo.size();
  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

    if( !cooldownInfo.OnCooldown() &&
        ( behavior->IsActivated() ||
          behavior->WantsToBeActivated(behaviorExternalInterface) ) ) {
      availableBehaviors.PushBack(behavior, _weights[idx]);

      PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.ConsiderBehavior",
                     "%s: considering '%s' with weight %f",
                     GetIDStr().c_str(),
                     behavior->GetIDStr().c_str(),
                     _weights[idx]);
    }
    else {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDispatcherRandom.IgnoreBehavior",
                     "%s: skipping '%s'",
                     GetIDStr().c_str(),
                     behavior->GetIDStr().c_str());
    }
  }

  if( !availableBehaviors.Empty() ) {
    return availableBehaviors.Sample(GetRNG());
  }
  else {
    
    // no behaviors were available, maybe it was because of cooldown? Go through again and find the one
    // closest to being off cooldown and return that
    float minCooldownRemaining = std::numeric_limits<float>::max();
    ICozmoBehaviorPtr bestBehavior;

    for( size_t idx = 0; idx < numBehaviors; ++idx ) {
      const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);    

      if( ( behavior->IsActivated() ||
            behavior->WantsToBeActivated(behaviorExternalInterface) ) ) {
        auto& cooldownInfo = _cooldownInfo.at(idx);
        const float remCooldown = cooldownInfo.GetRemainingCooldownTime_s();
        if( remCooldown < minCooldownRemaining ) {
          minCooldownRemaining = remCooldown;
          bestBehavior = behavior;
        }
      }
    }

    if( bestBehavior != nullptr ) {
      PRINT_CH_INFO("Behaviors",
                    "BehaviorDispatcherRandom.AllOnCooldown",
                    "%s: all behaviors on cooldown, so selecting '%s' which is closest to being off cooldown (%f secs)",
                    GetIDStr().c_str(),
                    bestBehavior->GetIDStr().c_str(),
                    minCooldownRemaining);
    }

    return bestBehavior;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRandom::DispatcherUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _shouldEndAfterBehavior &&
      ! IsControlDelegated() ) {
    CancelSelf();
  }
}


}
}
