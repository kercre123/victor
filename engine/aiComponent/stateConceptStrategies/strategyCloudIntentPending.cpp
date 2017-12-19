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

#include "engine/aiComponent/stateConceptStrategies/strategyCloudIntentPending.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudReceiver.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"


namespace Anki {
namespace Cozmo {

namespace{
const char* kCloudIntentKey = "cloudIntent";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value StrategyCloudIntentPending::GenerateCloudIntentPendingConfig(CloudIntent intent)
{
  Json::Value config = IStateConceptStrategy::GenerateBaseStrategyConfig(StateConceptStrategyType::CloudIntentPending);
  config[kCloudIntentKey] = CloudIntentToString(intent);
  return config;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyCloudIntentPending::StrategyCloudIntentPending(const Json::Value& config)
: IStateConceptStrategy(config)
{
  {
    const auto& intentStr = JsonTools::ParseString(config,
                                                   kCloudIntentKey,
                                                   "StrategyCloudIntentPending.ConfigError.NoIntent");
    _intent = CloudIntentFromString(intentStr);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyCloudIntentPending::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetAIComponent().GetBehaviorComponent().GetCloudReceiver().IsIntentPending(_intent);
}

}
}
