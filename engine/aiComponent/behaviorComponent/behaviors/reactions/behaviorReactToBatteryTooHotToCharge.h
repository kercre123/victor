/**
 * File: BehaviorReactToBatteryTooHotToCharge.h
 *
 * Author: Arjun Menon
 * Created: 2019-02-01
 *
 * Description: simple animation reaction behavior that listens for when the robot 
 * disconnects its battery upon being placed on the charger (due to high temperature) 
 * and needs to inform the user of the high temp warning
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBatteryTooHotToCharge__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBatteryTooHotToCharge__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToBatteryTooHotToCharge : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToBatteryTooHotToCharge();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToBatteryTooHotToCharge(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();

    struct Persistent {
      bool lastDisconnectedBecauseTooHot = false;
      bool animationPending = false;
    } persistent;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBatteryTooHotToCharge__
