/**
* File: conditionSimpleMood.h
*
* Author:  ross
* Created: apr 11 2018
*
* Description: True if the SimpleMood takes on a given value, or if it transitions from one value
*              to another this tick, based on config
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionSimpleMood_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionSimpleMood_H__
#pragma once

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {
  
enum class SimpleMoodType : uint8_t;

class ConditionSimpleMood : public IBEICondition
{
public:
  explicit ConditionSimpleMood(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  bool _useTransition;
  SimpleMoodType _mood;
  SimpleMoodType _from;
  SimpleMoodType _to;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionSimpleMood_H__
