/**
 * File: AIGoalStrategyObjectTapInteraction
 *
 * Author: Al Chaussee
 * Created: 10/28/2016
 *
 * Description: Specific strategy for doing things with a double tapped object.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyObjectTapInteraction_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyObjectTapInteraction_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

#include <vector>

namespace Anki {
namespace Cozmo {

  class Robot;
  class IBehavior;
    
  class AIGoalStrategyObjectTapInteraction : public IAIGoalStrategy
  {
  public:
    
    // Constructor
    AIGoalStrategyObjectTapInteraction(Robot& robot, const Json::Value& config);
    
    // true when this goal would be happy to start, false if it doens't want to be fired now
    virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override { return false; }
    
    // true when this goal wants to finish, false if it would rather continue
    virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override { return false; }
    
  };
  
} // namespace
} // namespace

#endif // endif
