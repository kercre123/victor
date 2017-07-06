/**
* File: strategyInNeedsBracket.cpp
*
* Author: Brad Neuman - Kevin M. Karol
* Created: 2017-06-20 - 2017-07-05
*
* Description: Strategy which wants to run whenever the specified need is in
* the specified need bracket
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/strategyInNeedsBracket.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kNeedIDKey = "need";
const char* kNeedBracketKey = "needBracket";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyInNeedsBracket::StrategyInNeedsBracket(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
  {
    const auto& needStr = JsonTools::ParseString(config,
                                                 kNeedIDKey,
                                                 "StrategyInNeedsBracket.ConfigError.Need");
    _need = NeedIdFromString(needStr);
  }

  {
    const auto& needBracketStr = JsonTools::ParseString(config,
                                                        kNeedBracketKey,
                                                        "StrategyInNeedsBracket.ConfigError.NeedLevel");
    _requiredBracket = NeedBracketIdFromString(needBracketStr);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyInNeedsBracket::WantsToRunInternal(const Robot& robot) const
{
  const bool inBracket = InRequiredNeedBracket(robot);
  return inBracket;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyInNeedsBracket::InRequiredNeedBracket(const Robot& robot) const
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool inBracket = currNeedState.IsNeedAtBracket(_need, _requiredBracket);
  return inBracket;
}


}
}
