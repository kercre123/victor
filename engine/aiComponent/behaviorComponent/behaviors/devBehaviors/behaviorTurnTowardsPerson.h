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

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/salientPointTypes.h"

namespace Anki {
namespace Cozmo {

class ConditionSalientPointDetected;

class BehaviorTurnTowardsPerson : public ICozmoBehavior
{
public: 
  virtual ~BehaviorTurnTowardsPerson() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTurnTowardsPerson(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

private:

  void TransitionToTurnTowardsPoint();
  void TransitionToFinishedTurning();
  void TransitionToCompleted();
  void BlinkLight(bool on);

  enum class State : uint8_t {
    Starting=0,
    Turning,
    FinishedTurning,
    Completed
  };

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    // after seeing a person, wait for this amount of seconds before reacting again
    float coolDownTime = 2.0;
  };

  struct DynamicVariables {
    DynamicVariables();
    void Reset();
    State state;

    Vision::SalientPoint lastPersonDetected;
    bool blinkOn;
    bool hasToStop;
  };


  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__
