/**
 * File: strategyTimer.h
 *
 * Author: Brad Neuman
 * Created: 2017-12-10
 *
 * Description: Simple strategy to become true a given time after a reset
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_ConditionTimer_H__
#define __Engine_AiComponent_StateConceptStrategies_ConditionTimer_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionTimer : public IBEICondition
{
public:
  explicit ConditionTimer(const float timeout_s);
  explicit ConditionTimer(const Json::Value& config);

  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  const float _timeout_s;
  float _timeToEnd_s = -1.0f;
};


} // namespace
} // namespace



#endif
