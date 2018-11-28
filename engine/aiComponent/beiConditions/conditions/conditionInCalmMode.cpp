/**
 * File: conditionInCalmMode.h
 *
 * Author: Guillermo Bautista
 * Created: 2018/11/26
 *
 * Description: Condition that indicates when Vector's SysCon is in Calm Mode.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionInCalmMode.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/powerStateManager.h"

namespace Anki {
namespace Vector {

ConditionInCalmMode::ConditionInCalmMode(const Json::Value& config)
: IBEICondition(config)
{
}

bool ConditionInCalmMode::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& powerSaveManager = behaviorExternalInterface.GetPowerStateManager();
  return powerSaveManager.InSysconCalmMode();
}


} // namespace Vector
} // namespace Anki
