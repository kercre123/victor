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

#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config)
  : IBehaviorDispatcher(config)
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
      
      const BehaviorID behaviorID = BehaviorIDFromString(behaviorIDStr);
      IBehaviorDispatcher::AddPossibleDispatch(behaviorID);

      const float cooldown = JsonTools::ParseFloat(
        behaviorDefinitionGroup,
        "cooldown_s",
        "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorGroup.NoCooldown");

      const float randomFactor = behaviorDefinitionGroup.get("cooldown_random_factor", 0.0f).asFloat();

      _cooldownInfo.emplace_back( CooldownInfo{._cooldown_s = cooldown, ._randomCooldownFactor = randomFactor} );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriorityWithCooldown::GetDesiredBehavior(
  BehaviorExternalInterface& behaviorExternalInterface)
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
          behavior->WantsToBeActivated(behaviorExternalInterface) ) ) {
      // we will dispatch to this behavior, so put it on cooldown
      // TEMP: cooldown should be when behavior stops, not here
      cooldownInfo.StartCooldown(GetRNG());
      return behavior;
    }
  }

  return ICozmoBehaviorPtr{};
}

bool BehaviorDispatcherStrictPriorityWithCooldown::CooldownInfo::OnCooldown() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return _onCooldownUntil_s >= 0.0f && currTime_s < _onCooldownUntil_s;
}

void BehaviorDispatcherStrictPriorityWithCooldown::CooldownInfo::StartCooldown(Util::RandomGenerator& rng)
{
  if( _cooldown_s > 0.0f ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    float cooldown_s = _cooldown_s;
    if( _randomCooldownFactor > 0.0f ) {
      cooldown_s *= rng.RandDblInRange( (1.0f - _randomCooldownFactor) * _cooldown_s,
                                        (1.0f + _randomCooldownFactor) * _cooldown_s );
    }
    
    _onCooldownUntil_s = currTime_s + _cooldown_s;
  }
}

}
}
