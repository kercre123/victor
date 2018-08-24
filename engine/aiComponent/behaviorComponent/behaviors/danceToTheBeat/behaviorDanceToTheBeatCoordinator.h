/**
 * File: BehaviorDanceToTheBeatCoordinator.h
 *
 * Author: Matt Michini
 * Created: 2018-08-20
 *
 * Description: Coordinates listening for beats and dancing to the beat
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDanceToTheBeatCoordinator__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDanceToTheBeatCoordinator__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorDanceToTheBeatCoordinator : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDanceToTheBeatCoordinator();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDanceToTheBeatCoordinator(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig() {};
    
    std::string listeningBehaviorStr;
    std::string longListeningBehaviorStr;
    std::string offChargerDancingBehaviorStr;
    std::string onChargerDancingBehaviorStr;
    
    ICozmoBehaviorPtr listeningBehavior;
    ICozmoBehaviorPtr longListeningBehavior;
    ICozmoBehaviorPtr offChargerDancingBehavior;
    ICozmoBehaviorPtr onChargerDancingBehavior;
    
    ICozmoBehaviorPtr driveOffChargerBehavior;
    ICozmoBehaviorPtr goHomeBehavior;
  };

  struct DynamicVariables {
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  // After listening is complete, check if there is a confident beat or not. If so, dispatch the proper behaviors
  void CheckIfBeatDetected();
  
  // Start dancing (while off charger). Optionally attempt to return to the charger after
  void TransitionToOffChargerDance(const bool returnToChargerAfter = false);
  
  // Start dancing (while on charger)
  void TransitionToOnChargerDance();
  
  // Drive off the charger to possibly dance
  void TransitionToDriveOffCharger();
  
  // After driving off the charger, listening to confirm a beat again
  void TransitionToOffChargerListening();
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDanceToTheBeatCoordinator__
