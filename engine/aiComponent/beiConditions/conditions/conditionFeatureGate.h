/**
 * File: conditionFeatureGate.cpp
 *
 * Author: ross
 * Created: 2018 Apr 11
 *
 * Description: True if a feature is enabled
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionFeatureGate_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionFeatureGate_H__
#pragma once

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {
  
enum class FeatureType : uint8_t;

class ConditionFeatureGate : public IBEICondition
{
public:
  ConditionFeatureGate(const Json::Value& config, BEIConditionFactory& factory);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
  FeatureType _featureType;
  bool _expected;
};

}
}


#endif
