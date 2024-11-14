/**
 * File: conditionMotionDetectedWithProx.h
 *
 * Author: Guillermo Bautista
 * Created: 04/24/2019
 *
 * Description: Condition which is true when motion is detected with the TOF sensor(s)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_ConditionMotionDetectedWithProx_H__
#define __Engine_AiComponent_StateConceptStrategies_ConditionMotionDetectedWithProx_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionMotionDetectedWithProx : public IBEICondition
{
public:
  explicit ConditionMotionDetectedWithProx(const Json::Value& config);
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  struct Params {
  };

  // Params _params;  
};


} // namespace
} // namespace



#endif
