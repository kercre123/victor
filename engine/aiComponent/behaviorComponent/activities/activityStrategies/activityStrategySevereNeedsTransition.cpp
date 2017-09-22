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

#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategySevereNeedsTransition.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategySevereNeedsTransition::ActivityStrategySevereNeedsTransition(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, config)
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategySevereNeedsTransition::WantsToStartInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityRanSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategySevereNeedsTransition.WantsToStartInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(behaviorExternalInterface);
  }
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategySevereNeedsTransition::WantsToEndInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityStartedSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategySevereNeedsTransition.WantsToEndInternal",
                 "WantsToRunStrategyNotSpecified")){
    return !_wantsToRunStrategy->WantsToRun(behaviorExternalInterface);
  }
  return true;
}

}
}
