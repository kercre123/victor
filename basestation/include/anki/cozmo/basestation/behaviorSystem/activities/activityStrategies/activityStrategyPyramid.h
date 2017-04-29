/**
 * File: activityStrategyPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 2/15/2017
 *
 * Description: Strategy which determines when the pyramid chooser will run in freeplay
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyPyramid_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyPyramid_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ActivityStrategyPyramid : public IActivityStrategy
{
public:

  // Constructor
  ActivityStrategyPyramid(Robot& robot, const Json::Value& config);

  // true when this activity would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const override;

  // true when this activity wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const override;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  mutable float _nextTimeUpdateRunability_s;
  mutable bool _wantsToRunRandomized;
  mutable bool _pyramidBuilt;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  
};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyPyramid_H__
