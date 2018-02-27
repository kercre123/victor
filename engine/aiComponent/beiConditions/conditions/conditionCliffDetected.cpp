/**
 * File: conditionCliffDetected.cpp
 *
 * Author: Matt Michini
 * Created: 2018/02/21
 *
 * Description: Condition that indicates when victor stops at a detected cliff
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionCliffDetected.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

namespace Anki {
namespace Cozmo {

ConditionCliffDetected::ConditionCliffDetected(const Json::Value& config)
: IBEICondition(config)
{
}


bool ConditionCliffDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& cliffSensorComponent = behaviorExternalInterface.GetRobotInfo().GetCliffSensorComponent();
  return cliffSensorComponent.IsCliffDetected();
}


} // namespace Cozmo
} // namespace Anki
