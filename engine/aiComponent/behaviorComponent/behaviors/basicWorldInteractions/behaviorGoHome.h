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
class BehaviorPickUpCube;
  
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
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
private:

  struct InstanceConfig {
    InstanceConfig() {
      homeFilter = std::make_unique<BlockWorldFilter>();
    }
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    AnimationTrigger leftTurnAnimTrigger    = AnimationTrigger::Count;
    AnimationTrigger rightTurnAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger backupStartAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger backupEndAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger backupLoopAnimTrigger  = AnimationTrigger::Count;
    AnimationTrigger raiseLiftAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger nuzzleAnimTrigger      = AnimationTrigger::Count;

    bool useCliffSensorCorrection = true;
    std::unique_ptr<BlockWorldFilter> homeFilter;
    
    int driveToRetryCount = 0;
    int turnToDockRetryCount = 0;
    int mountChargerRetryCount = 0;
    std::shared_ptr<BehaviorPickUpCube> pickupBehavior;
  };

  struct DynamicVariables {
    DynamicVariables() {};
    ObjectID chargerID;
    bool     drivingAnimsPushed = false;;
    int      driveToRetryCount = 0;
    int      turnToDockRetryCount = 0;
    int      mountChargerRetryCount = 0;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToCheckDockingArea();
  void TransitionToPlacingCubeOnGround();
  void TransitionToDriveToCharger();
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
