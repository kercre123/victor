/**
 * File: BehaviorDevViewCubeBackpackLights.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-24
 *
 * Description: A behavior which reads in a list of cube/backpack lights and exposes console vars to move through them
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevViewCubeBackpackLights__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevViewCubeBackpackLights__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "clad/types/backpackAnimationTriggers.h"
#include "clad/types/cubeAnimationTrigger.h"

namespace Anki {
namespace Vector {

class BehaviorDevViewCubeBackpackLights : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevViewCubeBackpackLights();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevViewCubeBackpackLights(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    std::vector<CubeAnimationTrigger> cubeTriggers;
    std::vector<BackpackAnimationTrigger> backpackTriggers;
  };

  struct DynamicVariables {
    DynamicVariables();
    u32 lastCubeTriggerIdx;
    u32 lastBackpackTriggerIdx;
    CubeAnimationTrigger lastCubeTrigger;
    ObjectID objectID;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevViewCubeBackpackLights__
