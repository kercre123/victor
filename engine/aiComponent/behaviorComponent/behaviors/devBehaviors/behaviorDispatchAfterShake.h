/**
 * File: behaviorDispatchAfterShake.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-18
 *
 * Description: Simple behavior to wait for the robot to be shaken and placed down before delegating to
 *              another data-defined behavior
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDispatchAfterShake_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDispatchAfterShake_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/engineTimeStamp.h"

namespace Anki {
namespace Vector {

class BehaviorDispatchAfterShake : public ICozmoBehavior
{
  friend class BehaviorFactory;
  explicit BehaviorDispatchAfterShake(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const final override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
private:
  struct InstanceConfig {
    InstanceConfig();
    std::vector<BehaviorID>        delegateIDs;
    std::vector<ICozmoBehaviorPtr> delegates;
  };

  struct DynamicVariables {
    DynamicVariables();
    size_t countShaken;
    bool   shakingSession; // with memory, not instantaneous
    float  lastChangeTime_s; // either the last time it was shaken, or the last time it was stopped
    EngineTimeStamp_t tickPlacedDown;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
};

} // namespace Vector
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDispatchAfterShake_H__
