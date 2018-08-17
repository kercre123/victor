/**
 * File: BehaviorReactToUncalibratedHeadAndLift.h
 *
 * Author: Arjun Menon
 * Created: 2018-08-10
 *
 * Description:
 * When the robot reports encoder readings outside
 * the range of physical expectations, calibrate these motors.
 * Note: This behavior should not run during certain power-saving
 * modes, and also only after a duration after being picked up
 * (to prevent calibration while being handled)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToUncalibratedHeadAndLift__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToUncalibratedHeadAndLift__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToUncalibratedHeadAndLift : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToUncalibratedHeadAndLift();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToUncalibratedHeadAndLift(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToUncalibratedHeadAndLift__
