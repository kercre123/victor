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

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFactory.h"

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityBehaviorsOnly.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityBuildPyramid.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFeeding.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFreeplay.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityGatherCubes.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activitySocialize.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activitySparked.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityVoiceCommand.h"

#include "anki/cozmo/basestation/robot.h"

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
    case ActivityType::VoiceCommand:
    {
      activityPtr = new ActivityVoiceCommand(robot, config);
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
