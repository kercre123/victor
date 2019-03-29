/**
 * File: conditionTooHotToCharge.h
 *
 * Author: Kevin Yoon
 * Created: 2019-02-07
 *
 * Description:
 * Returns whether or not the robot is on the charger and the battery temperature
 * so hot that charging cannot start
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionTooHotToCharge_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionTooHotToCharge_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionTooHotToCharge : public IBEICondition
{
public:
  explicit ConditionTooHotToCharge(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
};

}
}

#endif
