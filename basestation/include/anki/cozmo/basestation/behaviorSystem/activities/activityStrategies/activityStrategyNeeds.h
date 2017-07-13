/**
 * File: activityStrategyNeeds.h
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Strategy to start an activity based on being in a specific needs bracket
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyNeeds_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyNeeds_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"

#include "clad/types/needsSystemTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ActivityStrategyNeeds : public IActivityStrategy
{
public:
  ActivityStrategyNeeds(Robot& robot, const Json::Value& config);

  // Wants to run when in the specified needs bracket, and wants to end once it's not in that bracket anymore
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const override;
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const override;

private:
  IWantsToRunStrategyPtr _higherPriorityWantsToRunStrategy;
  
};

}
}

#endif
