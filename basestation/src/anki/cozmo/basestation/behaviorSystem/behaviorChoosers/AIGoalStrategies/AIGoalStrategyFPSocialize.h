/**
 * File: AIGoalStrategyFPSocialize
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay Socialize goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPSocialize_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPSocialize_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class AIGoalStrategyFPSocialize : public IAIGoalStrategy
{
public:

  // constructor
  AIGoalStrategyFPSocialize(Robot& robot, const Json::Value& config);

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override { return true; };

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // called whenever we detect an interaction of humans with Cozmo. Sets last timestamp
  void RegisterInteraction();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // after min duration, if there aren't interactions for this much time, the goal ends
  float _maxTimeWithoutInteractionSecs;
  
  // time at which we registered the last interaction
  float _lastInteractionTimeStamp;
};
  
} // namespace
} // namespace

#endif // endif
