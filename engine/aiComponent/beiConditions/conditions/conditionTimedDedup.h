/**
 * File: conditionTimedDedup.h
 *
 * Author: Kevin M. Karol
 * Created: 1/24/18
 *
 * Description: Condition which deduplicates its sub conditions results
 * Once the subcondition returns true once, the deduper will return false
 * for the specified interval, and then return the sub conditions results again
 * This makes it easy to toggle states based on conditions without worrying about
 * throttling back and forth on a per-tick basis
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionTimedDedup_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionTimedDedup_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/engineTimeStamp.h"
#include "coretech/common/shared/types.h"

namespace Anki {
namespace Vector {

class ConditionTimedDedup : public IBEICondition
{
public:
  explicit ConditionTimedDedup(const Json::Value& config);
  explicit ConditionTimedDedup(IBEIConditionPtr subCondition,
                               float dedupInterval_ms,
                               const std::string& ownerDebugLabel);

  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
private:
  struct {
    IBEIConditionPtr subCondition;
    float dedupInterval_ms = 0;
  } _instanceParams;

  mutable struct {
   EngineTimeStamp_t nextTimeValid_ms = 0;
  } _lifetimeParams;

};

}
}


#endif
