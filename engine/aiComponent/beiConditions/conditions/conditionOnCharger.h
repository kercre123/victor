/**
 * File: conditionOnCharger.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: True if the robot is on the charger contacts (not the platform)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionOnCharger_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionOnCharger_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionOnCharger : public IBEICondition
{
public:
  explicit ConditionOnCharger(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;
};

}
}


#endif
