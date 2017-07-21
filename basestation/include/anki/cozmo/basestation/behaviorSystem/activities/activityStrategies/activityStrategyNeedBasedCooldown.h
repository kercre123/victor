/**
 * File: ActivityStrategySinging.h
 *
 * Author: Al Chaussee
 * Created: 07/17/2017
 *
 * Description: Activity strategy for singing. Makes Cozmo more likely to sing when his Play need is high
 *              by lowering the strategy cooldown
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyNeedBasedCooldown_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyNeedBasedCooldown_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"

#include "clad/types/needsSystemTypes.h"
#include "json/json-forwards.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class ActivityStrategyNeedBasedCooldown : public IActivityStrategy
{
public:
  
  ActivityStrategyNeedBasedCooldown(Robot& robot, const Json::Value& config);
  
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const override { return true; }
  
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const override { return false; }
  
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  
  Util::GraphEvaluator2d _needCooldownGraph;
  Util::GraphEvaluator2d _needCooldownRandomnessGraph;
  
  NeedId _needId = NeedId::Count;
  
  std::vector<Signal::SmartHandle> _eventHandles;
};

}
}

#endif // endif __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyNeedBasedCooldown_H__
