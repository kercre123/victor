/**
 * File: conditionAnyUserIntent.cpp
 *
 * Author: Brad Neuman
 * Created: 2019-04-03
 *
 * Description: Simple condition for when any user intent is active or pending
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionAnyUserIntent.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

namespace Anki {
namespace Vector {

ConditionAnyUserIntent::ConditionAnyUserIntent(const Json::Value& config)
  : IBEICondition(config)
{
}

ConditionAnyUserIntent::~ConditionAnyUserIntent()
{
}

bool ConditionAnyUserIntent::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

  const bool pending = uic.IsAnyUserIntentPending();
  const bool active = uic.IsAnyUserIntentActive();

  return pending || active;
}


}
}
