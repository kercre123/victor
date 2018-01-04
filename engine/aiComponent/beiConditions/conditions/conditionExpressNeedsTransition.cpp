/**
* File: strategyExpressNeedsTransition.cpp
*
* Author: Brad Neuman - Kevin M. Karol
* Created: 2017-06-20 - 2017-07-05
*
* Description: Strategy which wants to run when cozmo's need bracket and expressed
* need state differ from each other
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionExpressNeedsTransition.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kNeedIDKey = "need";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionExpressNeedsTransition::ConditionExpressNeedsTransition(const Json::Value& config)
: IBEICondition(config)
{
  const auto& needStr = JsonTools::ParseString(config,
                                               kNeedIDKey,
                                               "ConditionExpressNeedsTransition.ConfigError.Need");
  _need = NeedIdFromString(needStr);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionExpressNeedsTransition::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& severeNeedsComponent = behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent();

  const bool inBracket = InRequiredNeedBracket(behaviorExternalInterface);
  const bool isBeingExpressed = severeNeedsComponent.GetSevereNeedExpression() == _need;

  return inBracket && !isBeingExpressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionExpressNeedsTransition::InRequiredNeedBracket(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(behaviorExternalInterface.HasNeedsManager()){
    auto& needsManager = behaviorExternalInterface.GetNeedsManager();
    NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();
    const bool inBracket = currNeedState.IsNeedAtBracket(_need, NeedBracketId::Critical);
    return inBracket;
  }
  return false;
}

}
}
