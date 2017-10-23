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

#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategyNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/stateConceptStrategyFactory.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kHigherPriorityStrategyConfigKey = "higherPriorityStrategyConfig";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyNeeds::ActivityStrategyNeeds(BehaviorExternalInterface& behaviorExternalInterface,
                                             IExternalInterface* robotExternalInterface,
                                             const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, robotExternalInterface, config)
, _higherPriorityWantsToRunStrategy(nullptr)
{
  if(config.isMember(kHigherPriorityStrategyConfigKey)){
    const Json::Value& higherPriorityWantsToRunConfig = config[kHigherPriorityStrategyConfigKey];

    _higherPriorityWantsToRunStrategy.reset(
        StateConceptStrategyFactory::CreateStateConceptStrategy(behaviorExternalInterface,
                                                            robotExternalInterface,
                                                            higherPriorityWantsToRunConfig));
    ANKI_VERIFY(_higherPriorityWantsToRunStrategy->GetStrategyType() == StateConceptStrategyType::InNeedsBracket,
                "ActivityStrategyNeeds.Constructor.IncorrectStrategyType",
                "Created HigherPrioritiy strategy with type %s",
                StateConceptStrategyTypeToString(_higherPriorityWantsToRunStrategy->GetStrategyType()));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyNeeds::WantsToStartInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityRanSec) const
{
  if(ANKI_VERIFY(_stateConceptStrategy != nullptr,
                 "ActivityStrategyNeeds.WantsToStartInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _stateConceptStrategy->AreStateConditionsMet(behaviorExternalInterface);
  }
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyNeeds::WantsToEndInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityStartedSec) const
{
  if(ANKI_VERIFY(_stateConceptStrategy != nullptr,
                 "ActivityStrategyNeeds.WantsToEndInternal",
                 "WantsToRunStrategyNotSpecified")){
    // Special case - if repair falls into severe state while severe energy is active
    // energy should want to end so that severe repair can take over
    if(_higherPriorityWantsToRunStrategy != nullptr){
      if(_higherPriorityWantsToRunStrategy->AreStateConditionsMet(behaviorExternalInterface)){
        behaviorExternalInterface.GetAIComponent().GetNonConstSevereNeedsComponent().ClearSevereNeedExpression();
        return true;
      }
    }
    
    
    return !_stateConceptStrategy->AreStateConditionsMet(behaviorExternalInterface);
  }
  return true;
}

} // namespace Cozmo
} // namespace Anki
