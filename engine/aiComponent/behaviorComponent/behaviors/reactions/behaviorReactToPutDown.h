/**
 * File: BehaviorReactToPutDown.h
 *
 * Author: Guillermo Bautista
 * Created: 2018-09-26
 *
 * Description: Behavior for reacting when a user places the robot down on a flat surface in an upright position
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPutDown__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPutDown__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToPutDown : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToPutDown();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToPutDown(const Json::Value& config);  

  virtual void InitBehavior() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  

private:
  void TransitionToPlayingPutDownAnimation();
  void TransitionToHeadCalibration();
  void TransitionToPlayingWaitAnimation();

  struct InstanceConfig {
    InstanceConfig();
    
    ICozmoBehaviorPtr waitInternalBehavior;
  };

  struct DynamicVariables {
    DynamicVariables() {}
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPutDown__
