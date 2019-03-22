/**
 * File: BehaviorInitialHeldInPalmReaction.h
 *
 * Author: Guillermo Bautista
 * Created: 2019-03-18
 *
 * Description: Behavior that selects the appropriate animation to play when Vector was recently placed in a user's palm
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInitialHeldInPalmReaction__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInitialHeldInPalmReaction__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorInitialHeldInPalmReaction : public ICozmoBehavior
{
public: 
  virtual ~BehaviorInitialHeldInPalmReaction();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorInitialHeldInPalmReaction(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void InitBehavior() override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr joltInPalmReaction;
    ICozmoBehaviorPtr palmTiltReaction;
    
    ICozmoBehaviorPtr animSelectorBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool animWasInterrupted = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInitialHeldInPalmReaction__
