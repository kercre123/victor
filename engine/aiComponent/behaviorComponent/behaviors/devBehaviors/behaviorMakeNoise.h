/**
 * File: BehaviorMakeNoise.h
 *
 * Author: ross
 * Created: 2018-05-25
 *
 * Description: moves all motors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMakeNoise__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMakeNoise__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorMakeNoise : public ICozmoBehavior
{
public: 
  virtual ~BehaviorMakeNoise();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorMakeNoise(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void MakeNoise();

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };
  
  struct DynamicVariables {
    int lastFunction = 0;
    DynamicVariables();
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMakeNoise__
