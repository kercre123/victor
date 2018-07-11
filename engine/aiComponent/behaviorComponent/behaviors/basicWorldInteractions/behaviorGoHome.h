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
class BehaviorClearChargerArea;
class BehaviorRequestToGoHome;
class BehaviorWiggleOntoChargerContacts;
  
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
    std::shared_ptr<BehaviorClearChargerArea> clearChargerAreaBehavior;
    std::shared_ptr<BehaviorRequestToGoHome> requestHomeBehavior;
    std::shared_ptr<BehaviorWiggleOntoChargerContacts> wiggleOntoChargerBehavior;
  };

  struct DynamicVariables {
    DynamicVariables() {};
    ObjectID chargerID;
    bool     drivingAnimsPushed = false;;
    int      driveToRetryCount = 0;
    int      turnToDockRetryCount = 0;
    int      mountChargerRetryCount = 0;
    
    struct Persistent {
      std::set<float> activatedTimes; // set of basestation times at which we've been activated
    };
    Persistent persistent;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToCheckDockingArea();
  void TransitionToPlacingCubeOnGround();
  void TransitionToDriveToCharger();
  void TransitionToCheckPreTurnPosition();
  void TransitionToTurn();
  void TransitionToMountCharger();
  void TransitionToPlayingNuzzleAnim();
  void TransitionToOnChargerCheck();
  
  // An action failed such that we must exit the behavior, or
  // we're out of retries for action failures.
  // Optionally remove the charger from BlockWorld if we failed in
  // such a way that we definitely don't know where the charger is
  void ActionFailure(bool removeChargerFromBlockWorld = false);
  
  void PushDrivingAnims();
  void PopDrivingAnims();
  
  // Clears the nav map of obstacles in a rough circle
  // between the robot and the charger, with some padding.
  void ClearNavMapUpToCharger();

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorGoHome_H__
