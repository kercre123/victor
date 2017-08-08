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

#include "engine/behaviorSystem/activities/activityStrategies/activityStrategySevereNeedsTransition.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategySevereNeedsTransition::ActivityStrategySevereNeedsTransition(Robot& robot, const Json::Value& config)
: IActivityStrategy(robot, config)
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategySevereNeedsTransition::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategySevereNeedsTransition.WantsToStartInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(robot);
  }
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategySevereNeedsTransition::WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategySevereNeedsTransition.WantsToEndInternal",
                 "WantsToRunStrategyNotSpecified")){
    return !_wantsToRunStrategy->WantsToRun(robot);
  }
  return true;
}

}
}
