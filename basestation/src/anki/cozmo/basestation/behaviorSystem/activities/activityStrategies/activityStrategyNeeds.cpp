/**
 * File: activityStrategyNeeds.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/wantsToRunStrategyFactory.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kHigherPriorityStrategyConfigKey = "higherPriorityStrategyConfig";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyNeeds::ActivityStrategyNeeds(Robot& robot, const Json::Value& config)
: IActivityStrategy(robot, config)
, _higherPriorityWantsToRunStrategy(nullptr)
{
  if(config.isMember(kHigherPriorityStrategyConfigKey)){
    const Json::Value& higherPriorityWantsToRunConfig = config[kHigherPriorityStrategyConfigKey];
    _higherPriorityWantsToRunStrategy =
        WantsToRunStrategyFactory::CreateWantsToRunStrategy(robot,
                                                            higherPriorityWantsToRunConfig);
    ANKI_VERIFY(_higherPriorityWantsToRunStrategy->GetStrategyType() == WantsToRunStrategyType::InNeedsBracket,
                "ActivityStrategyNeeds.Constructor.IncorrectStrategyType",
                "Created HigherPrioritiy strategy with type %s",
                WantsToRunStrategyTypeToString(_higherPriorityWantsToRunStrategy->GetStrategyType()));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyNeeds::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategyNeeds.WantsToStartInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(robot);
  }
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyNeeds::WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ActivityStrategyNeeds.WantsToEndInternal",
                 "WantsToRunStrategyNotSpecified")){
    // Special case - if repair falls into severe state while severe energy is active
    // energy should want to end so that severe repair can take over
    if(_higherPriorityWantsToRunStrategy != nullptr){
      if(_higherPriorityWantsToRunStrategy->WantsToRun(robot)){
        robot.GetAIComponent().GetNonConstWhiteboard().ClearSevereNeedExpression();
        return true;
      }
    }
    
    
    return !_wantsToRunStrategy->WantsToRun(robot);
  }
  return true;
}

} // namespace Cozmo
} // namespace Anki
