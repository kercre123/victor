/**
 * File: BehaviorDevPickup.h
 *
 * Author: Matt Michini
 * Created: 2018-08-29
 *
 * Description: testing a new pickup cube method
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevPickup__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevPickup__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BlockWorldFilter;
  
class BehaviorDevPickup : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevPickup() {};

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevPickup(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {};
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override {};

private:

  void TransitionToDriveToInFrontOfCube();
  void TransitionToFineTurn();
  
  struct InstanceConfig {
    InstanceConfig();
    
    std::unique_ptr<BlockWorldFilter> cubesFilter;
  };

  struct DynamicVariables {
    ObservableObject* cubePtr = nullptr;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevPickup__
