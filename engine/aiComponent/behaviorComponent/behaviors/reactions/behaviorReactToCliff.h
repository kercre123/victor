/**
 * File: behaviorReactToCliff.h
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include <array>

namespace Anki {
namespace Cozmo {

class ICompoundAction;
  
class BehaviorReactToCliff : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToCliff(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
  virtual void BehaviorUpdate() override;

private:
  using base = ICozmoBehavior;
  
  void TransitionToPlayingStopReaction();
  void TransitionToPlayingCliffReaction();
  void TransitionToBackingUp();
  
  // Based on which cliff sensor(s) was tripped, select an appropriate pre-animation action
  CompoundActionSequential* GetCliffPreReactAction(uint8_t cliffDetectedFlags);
  
  enum class State {
    PlayingStopReaction,
    PlayingCliffReaction,
    BackingUp
  };
  
  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr stuckOnEdgeBehavior;
  };

  InstanceConfig _iConfig;

  struct DynamicVariables {
    DynamicVariables();
    bool quitReaction;
    State state;
    bool gotCliff;
    bool gotStop;
    bool shouldStopDueToCharger;
    bool wantsToBeActivated;

    struct Persistent {
      std::array<u16, CliffSensorComponent::kNumCliffSensors> cliffValsAtStart;
    };
    Persistent persistent;
  };
  
  DynamicVariables _dVars;
  
}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
