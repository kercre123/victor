/**
 * File: activityStrategyNeeds.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Strategy to start an activity based on being in a specific needs bracket
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyNeeds::ActivityStrategyNeeds(Robot& robot, const Json::Value& config)
  : Anki::Cozmo::IActivityStrategy(config)
{
  {
    const auto& needStr = JsonTools::ParseString(config,
                                                 "need",
                                                 "ActivityStrategyNeeds.ConfigError.Need");
    _need = NeedIdFromString(needStr);
  }

  {
    const auto& needBracketStr = JsonTools::ParseString(config,
                                                        "needBracket",
                                                        "ActivityStrategyNeeds.ConfigError.NeedLevel");
    _requiredBracket = NeedBracketIdFromString(needBracketStr);
  }
}

bool ActivityStrategyNeeds::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  const bool inBracket = InRequiredNeedBracket(robot);
  return inBracket;
}

bool ActivityStrategyNeeds::WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const
{
  const bool inBracket = InRequiredNeedBracket(robot);
  return !inBracket;
}

bool ActivityStrategyNeeds::InRequiredNeedBracket(const Robot& robot) const
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool inBracket = currNeedState.IsNeedAtBracket(_need, _requiredBracket);
  return inBracket;
}


}
}
