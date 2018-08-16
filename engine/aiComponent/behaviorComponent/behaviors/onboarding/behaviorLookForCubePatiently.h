/**
 * File: BehaviorLookForCubePatiently.h
 *
 * Author: ross
 * Created: 2018-08-15
 *
 * Description: a T3MP behavior to patiently face the current direction, turning infrequently.
 *              It's T3MP since more generic cube search behaviors are being worked on, but adding params
 *              to cover this "patient" case is more trouble than it's worth for 1.0
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookForCubePatiently__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookForCubePatiently__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
enum class AnimationTrigger : int32_t;

class BehaviorLookForCubePatiently : public ICozmoBehavior
{
public: 
  virtual ~BehaviorLookForCubePatiently();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorLookForCubePatiently(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void TurnAndLoopAnim();

  struct InstanceConfig {
    InstanceConfig();
    AnimationTrigger animLoop;
  };

  struct DynamicVariables {
    DynamicVariables();
    std::weak_ptr<IActionRunner> animAction;
    float cancelLoopTime_s;
    float lastAngle_rad;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookForCubePatiently__
