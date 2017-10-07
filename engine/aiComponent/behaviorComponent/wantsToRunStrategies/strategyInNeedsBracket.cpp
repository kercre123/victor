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

#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/strategyInNeedsBracket.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kNeedIDKey = "need";
const char* kNeedBracketKey = "needBracket";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyInNeedsBracket::StrategyInNeedsBracket(BehaviorExternalInterface& behaviorExternalInterface,
                                               IExternalInterface* robotExternalInterface,
                                               const Json::Value& config)
: IWantsToRunStrategy(behaviorExternalInterface, robotExternalInterface, config)
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
bool StrategyInNeedsBracket::WantsToRunInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool inBracket = InRequiredNeedBracket(behaviorExternalInterface);
  return inBracket;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyInNeedsBracket::InRequiredNeedBracket(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool inBracket = currNeedState.IsNeedAtBracket(_need, _requiredBracket);
  return inBracket;
}


}
}
