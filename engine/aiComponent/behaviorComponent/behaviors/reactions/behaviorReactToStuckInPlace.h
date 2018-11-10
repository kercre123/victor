/**
 * File: BehaviorReactToStuckInPlace.h
 *
 * Author: Guillermo Bautista
 * Created: 2018-11-07
 *
 * Description: Behavior for reacting to a situation where the robot has been trapped in a location due to external factors, such as getting caught on a cable or wire
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToStuckInPlace__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToStuckInPlace__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/utils/recentOccurrenceTracker.h"

namespace Anki {
namespace Vector {

class BehaviorReactToStuckInPlace : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToStuckInPlace() {};

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToStuckInPlace(const Json::Value& config);
  
  virtual void InitBehavior() override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:  
  bool ShouldExecuteEmergencyTurn() const;
  bool ShouldAskForHelp() const;
  
  void TransitionToEmergencyTurn();
  void TransitionToSlowlyBackUp();
  void TransitionToCheckIfStillStuck();
  void TransitionToAskForHelp();
  
  enum class State {
    SlowlyBackingUp,
    EmergencyPointTurn,
    CheckingIfStillStuck,
    AskingForHelp
  };
  
  void SetInternalState(State state, const std::string& stateName);
  
  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    // Tracking for when to trigger fallback/secondary actions
    RecentOccurrenceTracker activationHistoryTracker;
    
    float frequentActivationCheckWindow_sec = 0.f;
    RecentOccurrenceTracker::Handle frequentActivationHandle;
    
    float failureActivationCheckWindow_sec = 0.f;
    RecentOccurrenceTracker::Handle failureActivationHandle;
    
    float retreatDistance_mm = 0.f;
    float retreatSpeed_mmps = 0.f;
    
    float pointTurnAngle_rad = 0.f;
    std::unique_ptr<PathMotionProfile> pointTurnCustomMotionProfile;
    
    ICozmoBehaviorPtr askForHelpBehavior;
  };

  struct DynamicVariables {
    DynamicVariables() {}
    State state;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToStuckInPlace__
