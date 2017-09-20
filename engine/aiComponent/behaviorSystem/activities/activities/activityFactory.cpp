/**
 * File: activityFactory.cpp
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Factory for generating activities that the BehaviorManager interacts with
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorSystem/activities/activities/activityFactory.h"

#include "engine/aiComponent/behaviorSystem/activities/activities/activityBehaviorsOnly.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityBuildPyramid.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityExpressNeeds.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityFeeding.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityFreeplay.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityGatherCubes.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activitySocialize.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activitySparked.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityStrictPriority.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityVoiceCommand.h"

#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace ActivityFactory{
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivity* CreateActivity(BehaviorExternalInterface& behaviorExternalInterface,
                          ActivityType activityType,
                          const Json::Value& config)
{
  IActivity* activityPtr = nullptr;
  
  switch(activityType){
    case ActivityType::BehaviorsOnly:
    {
      activityPtr = new ActivityBehaviorsOnly(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::BuildPyramid:
    {
      activityPtr = new ActivityBuildPyramid(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::Feeding:
    {
      activityPtr = new ActivityFeeding(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::Freeplay:
    {
      activityPtr = new ActivityFreeplay(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::GatherCubes:
    {
      activityPtr = new ActivityGatherCubes(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::Socialize:
    {
      activityPtr = new ActivitySocialize(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::Sparked:
    {
      activityPtr = new ActivitySparked(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::StrictPriority:
    {
      activityPtr = new ActivityStrictPriority(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::VoiceCommand:
    {
      activityPtr = new ActivityVoiceCommand(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::NeedsExpression:
    {
      activityPtr = new ActivityExpressNeeds(behaviorExternalInterface, config);
      break;
    }
    case ActivityType::Count:
    {
      DEV_ASSERT(false,
                 "ActivityFactory.AttemptedToMakeInvalidActivity");
      break;
    }
  }
  
  return activityPtr;
}

  
} // namespace ActivityFactory
} // namespace Cozmo
} // namespace Anki
