/**
 * File: conditionOnCharger.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: True if the robot is on the charger platform
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionOnCharger.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

ConditionOnCharger::ConditionOnCharger(const Json::Value& config)
  : IBEICondition(config)
{
}

bool ConditionOnCharger::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool onCharger = bei.GetRobotInfo().IsOnChargerContacts();
  return onCharger;
}

}
}
