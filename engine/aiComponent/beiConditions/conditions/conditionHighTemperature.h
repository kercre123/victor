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

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionHighTemperature_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionHighTemperature_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionHighTemperature : public IBEICondition
{
public:
  explicit ConditionHighTemperature(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
};

}
}

#endif
