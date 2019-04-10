/**
 * File: conditionAnyUserIntent.h
 *
 * Author: Brad Neuman
 * Created: 2019-04-03
 *
 * Description: Simple condition for when any user intent is active or pending
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionAnyUserIntent_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionAnyUserIntent_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionAnyUserIntent : public IBEICondition
{
public:
  explicit ConditionAnyUserIntent(const Json::Value& config);

  virtual ~ConditionAnyUserIntent();

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {}
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
};


}
}

#endif
