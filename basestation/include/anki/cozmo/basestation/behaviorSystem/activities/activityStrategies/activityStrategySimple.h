/**
 * File: ActivityStrategySimple
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Basic implementation of strategy for activities that rely on their behaviors to know when they want
 *              to start or finish, since duplicating the logic of when behaviors should run is too complex. An activity
 *              with this strategy will generally start when given a chance, and behaviors will drive when it ends.
 *              Note it still inherits the duration timers from the base class.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategySimple_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategySimple_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ActivityStrategySimple : public IActivityStrategy
{
public:

  // Constructor
  ActivityStrategySimple(Robot& robot, const Json::Value& config);

  // true when this activity would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const override { return true; };

  // true when this activity wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const override { return false; };

};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategySimple_H__
