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

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:
  
  void TransitionToTurn();
  void TransitionToMountCharger();
  void TransitionToPlayingNuzzleAnim();
  void TransitionToOnChargerCheck();
  
  void PushDrivingAnims();
  void PopDrivingAnims();
  
  struct {
    bool useCliffSensorCorrection = true;
    AnimationTrigger leftTurnAnimTrigger    = AnimationTrigger::Count;
    AnimationTrigger rightTurnAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger backupStartAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger backupEndAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger backupLoopAnimTrigger  = AnimationTrigger::Count;
    AnimationTrigger raiseLiftAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger nuzzleAnimTrigger      = AnimationTrigger::Count;
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  std::unique_ptr<BlockWorldFilter> _homeFilter;
  
  ObjectID _chargerID;
  
  bool _drivingAnimsPushed = false;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorGoHome_H__
