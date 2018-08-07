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
namespace Vector {
  
class BehaviorReactToPlacedOnSlope : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPlacedOnSlope(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void OnBehaviorActivated() override;

private:

  // Check robot's pitch angle at the end of the behavior
  void CheckPitch();
  
  // Keeps track of whether or not the robot ended the behavior still inclined
  bool _endedOnInclineLastTime = false;
  
  // The last time the behavior was run
  double _lastBehaviorTime = 0.0;
  
}; // class BehaviorReactToPlacedOnSlope
  

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__
