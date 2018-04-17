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

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionBatteryLevel_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionBatteryLevel_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/batteryTypes.h"

namespace Anki {
namespace Cozmo {

class ConditionBatteryLevel : public IBEICondition
{
public:
  ConditionBatteryLevel(const Json::Value& config, BEIConditionFactory& factory);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
  BatteryLevel _targetBatteryLevel;
  
};

}
}


#endif
