/**
* File: strategyCloudIntentPending.h
*
* Author: Kevin M. Karol
* Created: 2017-10-31
*
* Description: Strategy that checks to see if the specified cloud intent
* is pending
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCloudIntentPending_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCloudIntentPending_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/behaviorComponent/cloudIntents.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ConditionCloudIntentPending : public IBEICondition
{
public:
  static Json::Value GenerateCloudIntentPendingConfig(CloudIntent intent);
  
  explicit ConditionCloudIntentPending(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  CloudIntent _intent;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCloudIntentPending_H__
