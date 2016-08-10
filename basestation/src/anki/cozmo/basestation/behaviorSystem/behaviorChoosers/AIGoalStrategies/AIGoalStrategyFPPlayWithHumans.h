/**
 * File: AIGoalStrategyFPPlayWithHumans
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay PlayWithHumans goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPPlayWithHumans_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPPlayWithHumans_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class AIGoalStrategyFPPlayWithHumans : public IAIGoalStrategy
{
public:

  // constructor
  AIGoalStrategyFPPlayWithHumans(Robot& robot, const Json::Value& config);

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override;

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // declaration for any event handler
  template<typename T>
  void HandleMessage(const T& msg);
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // last time Cozmo requested a game
  float _lastGameRequestTimestampSec;


};
  
} // namespace
} // namespace

#endif // endif
