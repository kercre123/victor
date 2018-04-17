/**
 * File: conditionOffTreadsState.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-22
 *
 * Description: Checks if the off treads state matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Cozmo {

ConditionOffTreadsState::ConditionOffTreadsState(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  const std::string& targetStateStr = JsonTools::ParseString(config, "targetState", "ConditionOffTreadsState.Config");  
  ANKI_VERIFY(OffTreadsStateFromString(targetStateStr, _targetState),
              "ConditionOffTreadsState.Config.IncorrectString",
              "%s is not a valid OffTreadsState",
              targetStateStr.c_str());
}

ConditionOffTreadsState::ConditionOffTreadsState(const OffTreadsState& targetState, BEIConditionFactory& factory)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::OffTreadsState), factory)
{
  _targetState = targetState;
}

bool ConditionOffTreadsState::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const OffTreadsState currState = bei.GetRobotInfo().GetOffTreadsState();
  return (currState == _targetState);
}

}
}
