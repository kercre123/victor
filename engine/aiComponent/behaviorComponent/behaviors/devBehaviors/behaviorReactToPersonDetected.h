/**
 * File: BehaviorReactToPersonDetected.h
 *
 * Author: Lorenzo Riano
 * Created: 2018-05-30
 *
 * Description: Reacts when a person is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPersonDetected__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPersonDetected__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class ConditionPersonDetected;

class BehaviorReactToPersonDetected : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToPersonDetected();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToPersonDetected(const Json::Value& config);  

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

private:

  void TurnTowardsPoint();
  void FinishedTurning();
  void TransitionToCompleted();

  enum class State : uint8_t {
    Starting=0,
    Turning,
    TurnedAndWaiting,
    Completed
  };

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    void Reset();
    State state;
    bool sawAPerson = false;
  };


  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPersonDetected__
