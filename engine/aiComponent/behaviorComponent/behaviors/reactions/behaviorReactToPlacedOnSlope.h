/**
 * File: behaviorReactToPlacedOnSlope.h
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Behavior for responding to robot being placed down on a slope or at a pitch angle away from horizontal.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__
#define __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToPlacedOnSlope : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToPlacedOnSlope(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  // Check robot's pitch angle at the end of the behavior
  void CheckPitch(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Keeps track of whether or not the robot ended the behavior still inclined
  bool _endedOnInclineLastTime = false;
  
  // The last time the behavior was run
  double _lastBehaviorTime = 0.0;
  
}; // class BehaviorReactToPlacedOnSlope
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__
