/**
 * File: conditionNegate.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: Condition which reversed the result of it's operand
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionNegate_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionNegate_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionNegate : public IBEICondition
{
public:
  explicit ConditionNegate(const Json::Value& config);
  explicit ConditionNegate(IBEIConditionPtr subCondition);

  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
private:

  IBEIConditionPtr _operand;
};

}
}


#endif
