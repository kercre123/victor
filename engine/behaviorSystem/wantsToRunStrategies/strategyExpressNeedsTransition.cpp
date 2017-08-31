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

#include "engine/behaviorSystem/wantsToRunStrategies/strategyExpressNeedsTransition.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
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
StrategyExpressNeedsTransition::StrategyExpressNeedsTransition(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
  const auto& needStr = JsonTools::ParseString(config,
                                               kNeedIDKey,
                                               "StrategyExpressNeedsTransition.ConfigError.Need");
  _need = NeedIdFromString(needStr);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyExpressNeedsTransition::WantsToRunInternal(const Robot& robot) const
{
  const auto& severeNeedsComponent = robot.GetAIComponent().GetSevereNeedsComponent();

  const bool inBracket = InRequiredNeedBracket(robot);  
  const bool isBeingExpressed = severeNeedsComponent.GetSevereNeedExpression() == _need;

  return inBracket && !isBeingExpressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyExpressNeedsTransition::InRequiredNeedBracket(const Robot& robot) const
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool inBracket = currNeedState.IsNeedAtBracket(_need, NeedBracketId::Critical);
  return inBracket;
}

}
}
