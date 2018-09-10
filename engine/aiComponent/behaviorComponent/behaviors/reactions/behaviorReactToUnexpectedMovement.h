/**
 * File: behaviorReactToUnexpectedMovement.h
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
namespace Vector {
  
class BehaviorReactToUnexpectedMovement : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToUnexpectedMovement(const Json::Value& config);

  // Updates the internal tracking history of how frequently this behavior is being activated
  // while the robot is in a similar pose.
  void UpdateActivationHistory();
  
  // Keeps track of how frequently the behavior has detected being activated too frequently
  void UpdateRepeatedActivationDetectionHistory();

  bool HasBeenReactivatedFrequently() const;

  bool ShouldAskForHelp() const;
  
  void ResetActivationHistory();

  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    float repeatedActivationCheckWindow_sec = 0.f;
    size_t numRepeatedActivationsAllowed = 0;
    Radians yawAngleWindow_rad = Radians(0.f);

    float retreatDistance_mm = 0.f;
    float retreatSpeed_mmps = 0.f;
    
    float repeatedActivationDetectionsCheckWindow_sec = 0.f;
    u32 numRepeatedActivationDetectionsAllowed = 0;
    ICozmoBehaviorPtr askForHelpBehavior;
  };
  
  struct DynamicVariables {
    DynamicVariables() {};
    struct Persistent {
      std::vector<float> activatedTimes_sec; // set of basestation times at which we've been activated
      std::vector<Radians> yawAnglesAtActivation_rad; // set of yaw angles when we've been activated
      
      // Set of basestation times at which the behavior detected it had been activated too frequently
      std::set<float> repeatedActivationDetectionTimes_sec;
    };
    Persistent persistent;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

protected:
  virtual bool WantsToBeActivatedBehavior() const override { return true; };

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override { };
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
