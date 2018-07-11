/**
 * File: conditionBatteryLevel
 *
 * Author: Matt Michini
 * Created: 2018-03-07
 *
 * Description: Checks if the battery level matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionBatteryLevel.h"

#include "clad/types/batteryTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Cozmo {

ConditionBatteryLevel::ConditionBatteryLevel(const Json::Value& config)
  : IBEICondition(config)
{
  const std::string& targetLevelStr = JsonTools::ParseString(config, "targetBatteryLevel", "ConditionBatteryLevel.Config");
  ANKI_VERIFY(BatteryLevelFromString(targetLevelStr, _targetBatteryLevel),
              "ConditionBatteryLevel.Config.IncorrectString",
              "%s is not a valid BatteryLevel",
              targetLevelStr.c_str());
}

bool ConditionBatteryLevel::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const auto currBatteryLevel = bei.GetRobotInfo().GetBatteryLevel();
  return (currBatteryLevel == _targetBatteryLevel);
}

}
}
