/**
 * File: AIGoalStrategySpark
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for Spark goals.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategySpark_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategySpark_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class AIGoalStrategySpark : public IAIGoalStrategy
{
public:

  // constructor
  AIGoalStrategySpark(Robot& robot, const Json::Value& config);

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override { return true; };

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override { return false;};
};
  
} // namespace
} // namespace

#endif // endif
