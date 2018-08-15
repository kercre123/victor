/**
 * File: conditionStuckOnEdge.h
 *
 * Author: Matt Michini
 * Created: 2018/02/21
 *
 * Description: Condition that indicates when victor is stuck on a table edge
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __AiComponent_BeiConditions_ConditionStuckOnEdge__
#define __AiComponent_BeiConditions_ConditionStuckOnEdge__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionStuckOnEdge : public IBEICondition
{
public:
  explicit ConditionStuckOnEdge(const Json::Value& config);

  virtual ~ConditionStuckOnEdge() {};

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
private:
  mutable float _onEdgeStartTime_s;
};


} // namespace Vector
} // namespace Anki

#endif // __AiComponent_BeiConditions_ConditionStuckOnEdge__
