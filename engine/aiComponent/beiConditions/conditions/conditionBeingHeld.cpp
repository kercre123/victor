/**
 * File: conditionBeingHeld.cpp
 *
 * Author: Kevin Yoon
 * Created: 2018-08-15
 *
 * Description: Checks if the IS_BEING_HELD state matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionBeingHeld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

ConditionBeingHeld::ConditionBeingHeld(const Json::Value& config)
  : IBEICondition(config)
{
  _shouldBeHeld = JsonTools::ParseBool(config, "shouldBeHeld", "ConditionBeingHeld.Config");
  _minTimeSinceChange_ms = config.get("minTimeSinceChange_ms", 0).asInt();
  _maxTimeSinceChange_ms = config.get("maxTimeSinceChange_ms", INT_MAX).asInt();
}

ConditionBeingHeld::ConditionBeingHeld(const bool shouldBeHeld,
                                       const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::BeingHeld))
  , _minTimeSinceChange_ms(0)
  , _maxTimeSinceChange_ms(INT_MAX)
{
  SetOwnerDebugLabel(ownerDebugLabel);
  _shouldBeHeld = shouldBeHeld;
}

bool ConditionBeingHeld::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  if ( bei.GetRobotInfo().IsBeingHeld() == _shouldBeHeld ) {
    const EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    const EngineTimeStamp_t lastChangedTime_ms = bei.GetRobotInfo().GetBeingHeldLastChangedTime_ms();
    return (currTime_ms - lastChangedTime_ms >= _minTimeSinceChange_ms) &&
           (currTime_ms - lastChangedTime_ms <= _maxTimeSinceChange_ms);
  }
  return false;
}

}
}
