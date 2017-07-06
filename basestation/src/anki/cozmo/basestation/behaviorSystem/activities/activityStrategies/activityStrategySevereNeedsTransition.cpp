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
#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
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
