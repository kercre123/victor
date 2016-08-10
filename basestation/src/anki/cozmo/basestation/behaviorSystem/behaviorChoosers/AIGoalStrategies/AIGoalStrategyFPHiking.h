/**
 * File: AIGoalStrategyFPHiking
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay Hiking goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPHiking_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFPHiking_H__

#include "iAIGoalStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class AIGoalStrategyFPHiking : public IAIGoalStrategy
{
public:

  // constructor
  AIGoalStrategyFPHiking(Robot& robot, const Json::Value& config);

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const override;

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const override;

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns true if Cozmo would have something to do if we did hiking
  bool HasSomethingToDo(const Robot& robot) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // if this flag is set, this goal will start/continue if there are no beacons
  bool _createBeacons;
  
  // if this flag is set, this goal will start/continue if there are interesting edges in the memory map
  bool _visitInterestingEdges;
  
  // if this flag is set, this goal will start/continue if there are usable (not unknown) cubes out of beacons
  bool _gatherUsableCubesOutOfBeacons;
};
  
} // namespace
} // namespace

#endif // endif
