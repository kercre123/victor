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
namespace Vector {

class ConditionRobotTouched : public IBEICondition
{
public:
  explicit ConditionRobotTouched(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive) override;

private:

  bool IsReceivingTouch(BehaviorExternalInterface& behaviorExternalInterface) const;

  // minimum time we must be touched before acknowledging
  // can be 0.0f to trigger for every gradation of touch
  float _kMinTouchTime;

  // if true, then wait until the time conditions are met and the button is released. Condition will be true
  // for the given time
  bool _waitForRelease = false;

  // if waiting for release, this is how long the condition will stay true after a release
  float _waitForReleaseTimeout_s = -1.0f;

  mutable float _timePressed_s = -1.0f;
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotTouched_H__

