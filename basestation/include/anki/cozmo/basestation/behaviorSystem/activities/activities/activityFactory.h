/**
* File: activityFactory.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Factory for generating activities that the BehaviorManager interacts with
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFactory_H__

#include "clad/types/activityTypes.h"

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
class IActivity;
class Robot;
  
namespace ActivityFactory{

IActivity* CreateActivity(Robot& robot, ActivityType activityType, const Json::Value& config);
  
}
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFactory_H__
