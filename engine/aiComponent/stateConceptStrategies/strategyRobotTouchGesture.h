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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotTouchGesture_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotTouchGesture_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

#include "clad/types/touchGestureTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class StrategyRobotTouchGesture : public IStateConceptStrategy
{
public:
  explicit StrategyRobotTouchGesture(const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  bool IsReceivingTouchGesture(BehaviorExternalInterface& behaviorExternalInterface) const;

  TouchGesture _targetTouchGesture;  

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotTouchGesture_H__

