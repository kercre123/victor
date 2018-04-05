/**
* File: conditionBehaviorTimer.h
*
* Author: ross
* Created: feb 23 2018
*
* Description: Condition that checks the state of a shared named BehaviorTimer
*
* Copyright: Anki, Inc. 2018
*
**/


#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionBehaviorTimer_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionBehaviorTimer_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Cozmo {


class ConditionBehaviorTimer : public IBEICondition
{
public:
  
  explicit ConditionBehaviorTimer(const Json::Value& config);
  ~ConditionBehaviorTimer();

protected:
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
private:
  
  std::string _timerName;
  float _cooldown_s;
};
  
} // namespace
} // namespace

#endif // endif __Engine_AiComponent_BeiConditions_Conditions_ConditionBehaviorTimer_H__
