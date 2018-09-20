/**
 * File: BehaviorAskForHelp.h
 *
 * Author: Guillermo Bautista
 * Created: 2018-09-04
 *
 * Description: Behavior that periodically plays a "distressed" animation because
 *              he's stuck and needs help from the user to get out of his situation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStuck__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStuck__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorAskForHelp : public ICozmoBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAskForHelp(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  // Insert any possible root-level json keys that this class is expecting.
  // Not used currently since class does not use the ctor's "config" argument.
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  // The conditions for activation are defined in the class's "config" JSON file,
  // and are constructed, init'd, and checked by the base `ICozmoBehavior` class.
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  void SetAnimTriggers();
  void TriggerGetInAnim();
  void TriggerIdleAnim();

  struct InstanceConfig {
    // Not used currently since there are no config variables to initialize.
    InstanceConfig() {}
    // Put configuration variables here if needed in future
  };

  struct DynamicVariables {
    DynamicVariables();
    
    float startOfMotionDetectedTime_s;
    float enablePowerSaveModeTime_s;
    AnimationTrigger getInTrigger;
    AnimationTrigger idleTrigger;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStuck__
