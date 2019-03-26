/**
 * File: BehaviorCountingAnimation.h
 *
 * Author: Andrew Stout
 * Created: 2018-11-19
 *
 * Description: A hopefully-reusable behavior that coordinates a counting animation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCountingAnimation__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCountingAnimation__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorCountingAnimation : public ICozmoBehavior
{
public:
  virtual ~BehaviorCountingAnimation();

  // must be called before activation.
  // target is the number to count to,
  // announcement is what Vector should say when he finishes counting. (Keep it short.)
  void SetCountTarget(uint32_t target, const std::string & announcement="");

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCountingAnimation(const Json::Value& config);

  virtual void InitBehavior() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // configuration variables here
    uint32_t slowLoopBeginSize_loops; // how many we want to count slow at the beginning - expressed in loops (1 loop = 2 counts)
    uint32_t slowLoopEndSize_loops; // how many we want to count slow at the end - expressed in loops (1 loop = 2 counts)
    uint32_t getInOddCounts; // how many counts the odd get-in has
    uint32_t getInEvenCounts;
  };

  struct DynamicVariables {
    DynamicVariables();
    // member variables here
    uint32_t target;
    std::string announcement;
    bool even;
    uint32_t slowLoopBeginNumLoops;
    uint32_t fastLoopNumLoops;
    uint32_t slowLoopEndNumLoops;
    bool setTargetCalled;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void SetCountParameters();

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCountingAnimation__
