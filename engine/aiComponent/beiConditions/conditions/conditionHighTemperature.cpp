/**
 * File: conditionHighTemperature
 *
 * Author: Arjun Menon
 * Created: 2018-08-17
 *
 * Description:
 * Returns whether or not the robot's CPU or battery temperature is high
 * enough that action must be taken to cool the system down.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionHighTemperature.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/battery/batteryComponent.h"

namespace Anki {
namespace Vector {
  
namespace
{
  // this temperature is recommended by hardware team
  // after which we should enter power save otherwise
  // the robot will have choppy performance
  const int kCPUOverheatingThreshold_degC = 90;

  // Must be between 45C (battery disconnect threshold) and 
  // 60C (system shutdown threshold), but far away enough
  // from 60C that there's some margin for the battery to heat
  // up further while he's frantically searching for and
  // docking to the charger.
  const int kBatteryOverheatingThreshold_degC = 50;
}

ConditionHighTemperature::ConditionHighTemperature(const Json::Value& config)
  : IBEICondition(config)
{
}

bool ConditionHighTemperature::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool hotCPU     = bei.GetRobotInfo().GetCpuTemperature_degC() >= kCPUOverheatingThreshold_degC;
  const bool hotBattery = bei.GetRobotInfo().GetBatteryComponent().GetBatteryTemperature_C() >= kBatteryOverheatingThreshold_degC;
  return hotCPU || hotBattery;
}

}
}
