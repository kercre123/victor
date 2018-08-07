/**
 * File: BehaviorWiggleOntoChargerContacts.h
 *
 * Author: Brad
 * Created: 2018-04-26
 *
 * Description: Perform a few small motions or wiggles to get back on the charger contacts if we got bumped off
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWiggleOntoChargerContacts__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWiggleOntoChargerContacts__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorWiggleOntoChargerContacts : public ICozmoBehavior
{
public: 
  virtual ~BehaviorWiggleOntoChargerContacts();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorWiggleOntoChargerContacts(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    int maxAttempts;
  };

  enum class State {
    BackingUp,
    MovingForward,
    VerifyingContacts
  };
  
  struct DynamicVariables {
    DynamicVariables();
    State state;
    int attempts;
    float contactTime_s;
  };

  void StartStateMachine();
  
  void TransitionToBackingUp();
  void TransitionToMovingForward();
  void TransitionToVerifyingContacts();

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWiggleOntoChargerContacts__
