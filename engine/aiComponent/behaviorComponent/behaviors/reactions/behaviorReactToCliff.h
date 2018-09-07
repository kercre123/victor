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
namespace Vector {

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
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  
  virtual void BehaviorUpdate() override;

private:
  using base = ICozmoBehavior;
  
  void TransitionToStuckOnEdge();
  void TransitionToPlayingCliffReaction();
  void TransitionToBackingUp();
  
  // Based on which cliff sensor(s) was tripped, create the appropriate reaction
  CompoundActionSequential* GetCliffReactAction(uint8_t cliffDetectedFlags);
  
  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    ICozmoBehaviorPtr stuckOnEdgeBehavior;
    
    float cliffBackupDist_mm;
    float cliffBackupSpeed_mmps;
    
    float stopEventTimeout_sec;
  };

  InstanceConfig _iConfig;

  float _dramaticCliffReactionPlayableTime_sec = 0.f;  

  struct DynamicVariables {
    DynamicVariables();
    bool quitReaction;
    bool gotStop;
    bool putDownOnCliff;
    bool wantsToBeActivated;
    struct Persistent {
      int numStops;
      float lastStopTime_sec;
      std::array<u16, CliffSensorComponent::kNumCliffSensors> cliffValsAtStart;
    };
    Persistent persistent;
  };
  
  DynamicVariables _dVars;
  
}; // class BehaviorReactToCliff
  

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
