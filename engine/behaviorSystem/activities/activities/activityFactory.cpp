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

#include "engine/behaviorSystem/activities/activities/activityFactory.h"

#include "engine/behaviorSystem/activities/activities/activityBehaviorsOnly.h"
#include "engine/behaviorSystem/activities/activities/activityBuildPyramid.h"
#include "engine/behaviorSystem/activities/activities/activityExpressNeeds.h"
#include "engine/behaviorSystem/activities/activities/activityFeeding.h"
#include "engine/behaviorSystem/activities/activities/activityFreeplay.h"
#include "engine/behaviorSystem/activities/activities/activityGatherCubes.h"
#include "engine/behaviorSystem/activities/activities/activitySocialize.h"
#include "engine/behaviorSystem/activities/activities/activitySparked.h"
#include "engine/behaviorSystem/activities/activities/activityStrictPriority.h"

#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace ActivityFactory{
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivity* CreateActivity(Robot& robot, ActivityType activityType, const Json::Value& config)
{
  IActivity* activityPtr = nullptr;
  
  switch(activityType){
    case ActivityType::BehaviorsOnly:
    {
      activityPtr = new ActivityBehaviorsOnly(robot, config);
      break;
    }
    case ActivityType::BuildPyramid:
    {
      activityPtr = new ActivityBuildPyramid(robot, config);
      break;
    }
    case ActivityType::Feeding:
    {
      activityPtr = new ActivityFeeding(robot, config);
      break;
    }
    case ActivityType::Freeplay:
    {
      activityPtr = new ActivityFreeplay(robot, config);
      break;
    }
    case ActivityType::GatherCubes:
    {
      activityPtr = new ActivityGatherCubes(robot, config);
      break;
    }
    case ActivityType::Socialize:
    {
      activityPtr = new ActivitySocialize(robot, config);
      break;
    }
    case ActivityType::Sparked:
    {
      activityPtr = new ActivitySparked(robot, config);
      break;
    }
    case ActivityType::StrictPriority:
    {
      activityPtr = new ActivityStrictPriority(robot, config);
      break;
    }
    case ActivityType::NeedsExpression:
    {
      activityPtr = new ActivityExpressNeeds(robot, config);
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
