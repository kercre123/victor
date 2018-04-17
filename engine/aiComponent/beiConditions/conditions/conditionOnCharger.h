/**
 * File: conditionOnCharger.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: True if the robot is on the charger platform
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionOnCharger_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionOnCharger_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionOnCharger : public IBEICondition
{
public:
  ConditionOnCharger(const Json::Value& config, BEIConditionFactory& factory);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;
};

}
}


#endif
