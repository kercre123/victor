/**
 * File: behaviorReactToRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToRobotOnFace : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnFace(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;  

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override;

private:
  
  struct DynamicVariables {
    // If true, we cancel the behavior if we detect that the OffTreadsState is no longer OnFace. Note that we set this
    // to false when the animation is about to flip the robot over, since at that point the OffTreadsState is expected
    // to change.
    bool cancelIfNotOnFace = true;
  };
  
  DynamicVariables _dVars;
  
  void FlipOverIfNeeded();
  void DelayThenCheckState();
  void CheckFlipSuccess();
};

}
}

#endif
