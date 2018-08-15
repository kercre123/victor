/**
* File: conditionTriggerWordPending.h
*
* Author: Kevin M. Karol
* Created: 2017-10-31
*
* Description: Strategy that checks to see if the trigger word is pending
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTriggerWordPending_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTriggerWordPending_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "json/json-forwards.h"

namespace Anki {
namespace Vector {

class ConditionTriggerWordPending : public IBEICondition
{
public:
  
  explicit ConditionTriggerWordPending(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTriggerWordPending_H__
