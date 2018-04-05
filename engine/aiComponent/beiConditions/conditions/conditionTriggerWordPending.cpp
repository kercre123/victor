/**
* File: strategyTriggerWordPending.cpp
*
* Author: Kevin M. Karol
* Created: 2017-10-31
*
* Description: Strategy that checks to see if the trigger word is pending
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionTriggerWordPending.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"


namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTriggerWordPending::ConditionTriggerWordPending(const Json::Value& config)
: IBEICondition(config)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionTriggerWordPending::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& uic = behaviorExternalInterface.GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
  return uic.IsTriggerWordPending();
}

}
}
