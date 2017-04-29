/**
 * File: ActivityStrategyObjectTapInteraction.h
 *
 * Author: Al Chaussee
 * Created: 10/28/2016
 *
 * Description: Specific strategy for doing things with a double tapped object.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyObjectTapInteraction_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyObjectTapInteraction_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"
#include "json/json-forwards.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;
class IBehavior;
  
class ActivityStrategyObjectTapInteraction : public IActivityStrategy
{
public:
  
  // Constructor
  ActivityStrategyObjectTapInteraction(Robot& robot, const Json::Value& config);
  
  // true when this activity would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const override { return false; }
  
  // true when this activity wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const override { return false; }
  
};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyObjectTapInteraction_H__
