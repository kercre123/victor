/**
 * File: BehaviorDevDesignCubeLights.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-20
 *
 * Description: A behavior which exposes console vars to design cube lights
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevDesignCubeLights__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevDesignCubeLights__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/components/cubes/cubeLights/cubeLightAnimation.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"

namespace Anki {
namespace Cozmo {

class BehaviorDevDesignCubeLights : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevDesignCubeLights();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevDesignCubeLights(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    CubeLightAnimation::ObjectLights currentLights;
    CubeLightComponent::AnimationHandle handle = 0;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  CubeLightAnimation::ObjectLights BuildLightsFromConsoleVars() const;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevDesignCubeLights__
