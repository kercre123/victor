/**
* File: strategyRobotTouched.h
*
* Author: Arjun Menon
* Created: 10/18/2017
*
* Description: 
* Strategy that wants to run whenever a robot is touched
* 
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouched_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouched_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ConditionRobotTouched : public IBEICondition
{
public:
  ConditionRobotTouched(const Json::Value& config, BEIConditionFactory& factory);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  bool IsReceivingTouch(BehaviorExternalInterface& behaviorExternalInterface) const;

  // minimum time we must be touched before acknowledging
  // can be 0.0f to trigger for every gradation of touch
  float _kMinTouchTime;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouched_H__

