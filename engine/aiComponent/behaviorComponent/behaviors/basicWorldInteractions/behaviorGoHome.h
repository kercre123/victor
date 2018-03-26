/**
* File: behaviorGoHome.h
*
* Author: Matt Michini
* Created: 11/1/17
*
* Description:
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Engine_Behaviors_BehaviorGoHome_H__
#define __Engine_Behaviors_BehaviorGoHome_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
  
class BehaviorGoHome : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorGoHome(const Json::Value& config);
  
public:
  virtual ~BehaviorGoHome() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Standard });
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    AnimationTrigger leftTurnAnimTrigger;
    AnimationTrigger rightTurnAnimTrigger;
    AnimationTrigger backupStartAnimTrigger;
    AnimationTrigger backupEndAnimTrigger;
    AnimationTrigger backupLoopAnimTrigger;
    AnimationTrigger raiseLiftAnimTrigger;
    AnimationTrigger nuzzleAnimTrigger;

    bool useCliffSensorCorrection;
    std::unique_ptr<BlockWorldFilter> homeFilter;
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID chargerID;
    bool     drivingAnimsPushed;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToTurn();
  void TransitionToMountCharger();
  void TransitionToPlayingNuzzleAnim();
  void TransitionToOnChargerCheck();
  void ActionFailure();
  
  void PushDrivingAnims();
  void PopDrivingAnims();
  
    
  

  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorGoHome_H__
