/**
 * File: activityStrategySevereNeedsTransition.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Strategy to start an activity when the needs system says we _should_ express a severe need,
 *              but we haven't started expressing it yet (according to the whiteboard)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySevereNeedsTransition.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategySevereNeedsTransition::ActivityStrategySevereNeedsTransition(Robot& robot, const Json::Value& config)
  : Anki::Cozmo::IActivityStrategy(config)
{
  {
    const auto& needStr = JsonTools::ParseString(config,
                                                 "need",
                                                 "ActivityStrategySevereNeedsTransition.ConfigError.Need");
    _need = NeedIdFromString(needStr);
  }
}

bool ActivityStrategySevereNeedsTransition::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  const auto& whiteboard = robot.GetAIComponent().GetWhiteboard();

  const bool inBracket = InRequiredNeedBracket(robot);  
  const bool isBeingExpressed = whiteboard.GetSevereNeedExpression() == _need;

  return inBracket && !isBeingExpressed;
}

bool ActivityStrategySevereNeedsTransition::WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const
{
  const bool inBracket = InRequiredNeedBracket(robot);
  return !inBracket;
}

bool ActivityStrategySevereNeedsTransition::InRequiredNeedBracket(const Robot& robot) const
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool inBracket = currNeedState.IsNeedAtBracket(_need, NeedBracketId::Critical);
  return inBracket;
}

}
}
