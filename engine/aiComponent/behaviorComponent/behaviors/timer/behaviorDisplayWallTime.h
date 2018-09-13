/**
 * File: behaviorDisplayWallTime.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-30
 *
 * Description: If the robot has a valid wall time, display it on the robot's face
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWallTime__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWallTime__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"
#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Vector {

class BehaviorDisplayWallTime : public BehaviorProceduralClock
{
public: 
  virtual ~BehaviorDisplayWallTime();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDisplayWallTime(const Json::Value& config);  

  virtual void GetBehaviorJsonKeysInternal(std::set<const char*>& expectedKeys) const override;

  virtual void TransitionToShowClockInternal() override;
  virtual bool WantsToBeActivatedBehavior() const override;

  virtual bool ShouldDimLeadingZeros() const override;

private:
  BehaviorProceduralClock::GetDigitsFunction BuildTimerFunction() const;

  // Checks to see whether to display the time as 12 vs 24 hour clock
  bool ShouldDisplayAsMilitaryTime() const;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWallTime__
