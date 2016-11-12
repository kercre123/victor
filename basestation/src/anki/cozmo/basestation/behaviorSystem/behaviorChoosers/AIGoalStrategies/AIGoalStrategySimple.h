/**
 * File: AIGoalStrategySimple
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Basic implementation of strategy for goals that rely on their behaviors to know when they want
 *              to start or finish, since duplicating the logic of when behaviors should run is too complex. A goal
 *              with this strategy will generally start when given a chance, and behaviors will drive when it ends.
 *              Note it still inherits the duration timers from the base class.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategySimple_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategySimple_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class AIGoalStrategySimple : public IAIGoalStrategy
{
public:

  // Constructor
  AIGoalStrategySimple(Robot& robot, const Json::Value& config);

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override { return true; };

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override { return false; };

};
  
} // namespace
} // namespace

#endif // endif
