/**
* File: strategyRobotTouchGesture.h
*
* Author: Arjun Menon
* Created: 10/18/2017
*
* Description: 
* Strategy that wants to run whenever a particular touch gesture is active
* 
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouchGesture_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouchGesture_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/touchGestureTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class ConditionRobotTouchGesture : public IBEICondition
{
public:
  explicit ConditionRobotTouchGesture(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  bool IsReceivingTouchGesture(BehaviorExternalInterface& behaviorExternalInterface) const;

  TouchGesture _targetTouchGesture;  

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouchGesture_H__

