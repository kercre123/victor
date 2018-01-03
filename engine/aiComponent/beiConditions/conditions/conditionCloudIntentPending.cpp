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

#include "engine/aiComponent/beiConditions/conditions/conditionCloudIntentPending.h"

#include "coretech/common/engine/jsonTools.h"
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
Json::Value ConditionCloudIntentPending::GenerateCloudIntentPendingConfig(CloudIntent intent)
{
  Json::Value config = IBEICondition::GenerateBaseConditionConfig(BEIConditionType::CloudIntentPending);
  config[kCloudIntentKey] = CloudIntentToString(intent);
  return config;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCloudIntentPending::ConditionCloudIntentPending(const Json::Value& config)
: IBEICondition(config)
{
  {
    const auto& intentStr = JsonTools::ParseString(config,
                                                   kCloudIntentKey,
                                                   "ConditionCloudIntentPending.ConfigError.NoIntent");
    _intent = CloudIntentFromString(intentStr);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionCloudIntentPending::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetAIComponent().GetBehaviorComponent().GetCloudReceiver().IsIntentPending(_intent);
}

}
}
