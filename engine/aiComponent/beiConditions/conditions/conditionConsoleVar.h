/**
* File: conditionConsoleVar.h
*
* Author: ross
* Created: feb 22 2018
*
* Description: Condition that is true when a console var matches a particular value
*
* Copyright: Anki, Inc. 2018
*
**/


#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionConsoleVar_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionConsoleVar_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {

namespace Util {
  class IConsoleVariable;
}
  
namespace Cozmo {


class ConditionConsoleVar : public IBEICondition
{
public:
  
  explicit ConditionConsoleVar(const Json::Value& config);
  ~ConditionConsoleVar();

protected:
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive) override;
  
private:
  
  std::string _variableName;
  size_t _expectedValue;
  
  const Anki::Util::IConsoleVariable* _var = nullptr;
};
  
} // namespace
} // namespace

#endif // endif __Engine_AiComponent_BeiConditions_Conditions_ConditionConsoleVar_H__
