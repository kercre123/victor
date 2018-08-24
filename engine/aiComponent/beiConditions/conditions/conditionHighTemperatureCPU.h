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

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionHighTemperatureCPU_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionHighTemperatureCPU_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionHighTemperatureCPU : public IBEICondition
{
public:
  explicit ConditionHighTemperatureCPU(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
};

}
}

#endif
