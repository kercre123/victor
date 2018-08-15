/**
 * File: conditionBecameTrueThisTick.cpp
 *
 * Author: ross
 * Created: Jun 5 2018
 *
 * Description: Condition that is true when a subcondition transitions from false to true this tick
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionBecameTrueThisTick_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionBecameTrueThisTick_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionBecameTrueThisTick : public IBEICondition
{
public:
  explicit ConditionBecameTrueThisTick(const Json::Value& config);
  
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
  
protected:
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
private:
  
  IBEIConditionPtr _subCondition;
  mutable int _lastResult = -1; // usually 0 or 1. when uninitialized, -1.
};

}
}


#endif
