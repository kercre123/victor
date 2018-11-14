/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnBack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnBack_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToRobotOnBack : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnBack(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override;
  
private:

  struct InstanceConfig {
    bool exitIfHeld = false;
  };
  
  InstanceConfig _iConfig;
  
  struct DynamicVariables {
    // If true, we cancel the behavior if we detect that the OffTreadsState is no longer OnBack. Note that we set this
    // to false when the animation is about to flip the robot down, since at that point the OffTreadsState is expected
    // to change.
    bool cancelIfNotOnBack = true;
  };
  
  DynamicVariables _dVars;

  void FlipDownIfNeeded();
  void DelayThenFlipDown();
  
};

}
}

#endif
