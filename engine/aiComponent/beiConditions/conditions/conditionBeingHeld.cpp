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
}

ConditionBeingHeld::ConditionBeingHeld(const bool shouldBeHeld,
                                       const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::BeingHeld))
{
  SetOwnerDebugLabel(ownerDebugLabel);
  _shouldBeHeld = shouldBeHeld;
}

bool ConditionBeingHeld::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  return (bei.GetRobotInfo().IsBeingHeld() == _shouldBeHeld);
}

}
}
