/**
* File: conditionTimePowerButtonPressed.cpp
*
* Author: Kevin M. Karol
* Created: 7/19/18
*
* Description: Condition that returns true when the power button has been held down
* for a sufficient length of time
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionTimePowerButtonPressed.h"

#include "coretech/common/engine/jsonTools.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

namespace {
const char* kTimeButtonPressed_ms  = "minTimeButtonPressed_ms";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTimePowerButtonPressed::ConditionTimePowerButtonPressed(const Json::Value& config)
: IBEICondition(config)
{
  _minTimePressed_ms = JsonTools::ParseUInt32(config, kTimeButtonPressed_ms, 
                                              "ConditionTimePowerButtonPressed.MissingKey.MinTimeButtonPressed");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTimePowerButtonPressed::ConditionTimePowerButtonPressed(const TimeStamp_t minTimePressed_ms, const std::string& ownerDebugLabel)
: IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::TimePowerButtonPressed))
, _minTimePressed_ms(minTimePressed_ms)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionTimePowerButtonPressed::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  return _minTimePressed_ms <= behaviorExternalInterface.GetRobotInfo().GetTimeSincePowerButtonPressed_ms();
}


} // namespace
} // namespace
