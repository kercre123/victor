/**
 * File: behaviorReactToRobotOnSide.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnSide_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnSide_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToRobotOnSide : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnSide(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
private:
  
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    // After having been activated for this long, then transition to the "ask for help" behavior. If this is less than
    // zero, then just infinitely loop in this behavior until we're no longer on our side.
    float askForHelpAfter_sec = -1.f;
    
    std::string askForHelpBehaviorStr;
    ICozmoBehaviorPtr askForHelpBehavior;
  };
  
  struct DynamicVariables
  {
    DynamicVariables() {};
    
    bool getOutPlayed = false;
  };
  
  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  void ReactToBeingOnSide();
  void HoldingLoop();
};

}
}

#endif
