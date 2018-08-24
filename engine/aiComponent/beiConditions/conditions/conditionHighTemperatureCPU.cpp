/**
 * File: conditionHighTemperatureCPU
 *
 * Author: Arjun Menon
 * Created: 2018-08-17
 *
 * Description:
 * Returns whether or not the robot's reported temperature is greater than 90
 * degrees C. Beyond this temperature, the robot's performance will be choppy
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionHighTemperatureCPU.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {
  
namespace
{
  // this temperature is recommended by hardware team
  // after which we should enter power save otherwise
  // the robot will have choppy performance
  const int kTemperatureThresholdOverheating_degC = 90;
}

ConditionHighTemperatureCPU::ConditionHighTemperatureCPU(const Json::Value& config)
  : IBEICondition(config)
{
}

bool ConditionHighTemperatureCPU::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool isHighTemp = bei.GetRobotInfo().GetCpuTemperature_degC() >= kTemperatureThresholdOverheating_degC;
  return isHighTemp;
}

}
}
