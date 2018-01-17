/**
 * File: conditionNot.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: Condition which reversed the result of it's sub-condition
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionNot_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionNot_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionNot : public IBEICondition
{
public:
  explicit ConditionNot(const Json::Value& config);
  explicit ConditionNot(IBEIConditionPtr subCondition);

  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
private:

  IBEIConditionPtr _subCondition;
};

}
}


#endif
