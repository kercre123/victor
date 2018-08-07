/**
 * File: behaviorTurn.h
 *
 * Author:  Kevin M. Karol
 * Created: 1/26/18
 *
 * Description:  Behavior which turns the robot a set number of degrees
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorTurn_H__
#define __Cozmo_Basestation_Behaviors_BehaviorTurn_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
class BehaviorTurn : public ICozmoBehavior
{
protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorTurn(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  
  
private:
  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    float turnRad;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

}; // class __Cozmo_Basestation_Behaviors_BehaviorTurn_H__

} // namespace Vector
} // namespace Anki

#endif
