/**
 * File: behaviorReactTo                 UnexpectedMovement.h
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/unexpectedMovementTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToUnexpectedMovement : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToUnexpectedMovement(const Json::Value& config);

  struct InstanceConfig {
      InstanceConfig() {};
      InstanceConfig(const Json::Value& config, const std::string& debugName);
      
      float repeatedActivationCheckWindow_sec = 0.f;
      size_t numRepeatedActivationsAllowed = 0;
      
      float retreatDistance_mm = 0.f;
      float retreatSpeed_mmps = 0.f;
  };
  
  struct DynamicVariables {
      DynamicVariables() {};
      struct Persistent {
          std::set<float> activatedTimes; // set of basestation times at which we've been activated
      };
      Persistent persistent;
  };
 
  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
    
  IBEIConditionPtr _unexpectedMovementCondition;
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override { };
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
