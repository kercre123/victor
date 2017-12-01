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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyCloudIntentPending_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyCloudIntentPending_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

#include "clad/types/behaviorComponent/cloudIntents.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class StrategyCloudIntentPending : public IStateConceptStrategy
{
public:
  static Json::Value GenerateCloudIntentPendingConfig(CloudIntent intent);
  
  explicit StrategyCloudIntentPending(const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  CloudIntent _intent;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyCloudIntentPending_H__
