/**
 * File: conditionIsMaintenanceReboot.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-09-04
 *
 * Description: Checks OS state to see if this is a maintenance reboot
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionIsMaintenanceReboot.h"

#include "osState/osState.h"

namespace Anki {
namespace Vector {

ConditionIsMaintenanceReboot::ConditionIsMaintenanceReboot(const Json::Value& config)
  : IBEICondition(config)
{
}
  
bool ConditionIsMaintenanceReboot::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool isReboot = OSState::getInstance()->RebootedForMaintenance();
  return isReboot;
}
  
} // namespace
} // namespace
