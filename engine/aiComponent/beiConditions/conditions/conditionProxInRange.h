/**
 * File: conditionProxInRange.h
 *
 * Author: Kevin M. Karol
 * Created: 1/26/18
 *
 * Description: Determine whether the current proximity sensor reading is valid
 * and within the given range
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_ConditionProxInRange_H__
#define __Engine_AiComponent_StateConceptStrategies_ConditionProxInRange_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include <limits>

namespace Anki {
namespace Vector {

class ConditionProxInRange : public IBEICondition
{
public:
  explicit ConditionProxInRange(const Json::Value& config);
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  struct Params {
    float minDist_mm = 0.0f;
    float maxDist_mm = std::numeric_limits<float>::max();
    // When embedding the condition inside other conditions (e.g. negate) there is
    // sometimes a desire to override the value returned on invalid sensor readings
    bool  invalidSensorReturn = false;
  };

  Params _params;  
};


} // namespace
} // namespace



#endif
