/**
 * File: behaviorStuckOnEdge.h
 *
 * Author: Kevin Yoon
 * Created: 2018-05-08
 *
 * Description: Behavior that periodically plays a "distressed" animation because
 *              he's stuck and needs help from the user to get out of his situation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorStuckOnEdge_H__
#define __Cozmo_Basestation_Behaviors_BehaviorStuckOnEdge_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorStuckOnEdge : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorStuckOnEdge(const Json::Value& config);
  
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

private:
  void TriggerGetInAnim();
  void TriggerIdleAnim();
  
  struct InstanceConfig{
    IBEIConditionPtr  stuckOnEdgeCondition;
  };

  struct DynamicVariables {
    DynamicVariables() = default;
    
    float startOfMotionDetectedTime_s = 0.0f;
    float enablePowerSaveModeTime_s = 0.0f;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

}
}

#endif
