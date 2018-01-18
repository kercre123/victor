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
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorGoHome(const Json::Value& config);
  
public:
  virtual ~BehaviorGoHome() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;

private:
  
  void TransitionToTurn();
  void TransitionToMountCharger();
  void TransitionToPlayingNuzzleAnim();
  
  struct {
    bool useCliffSensorCorrection = true;
    AnimationTrigger turningAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger backupAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger nuzzleAnimTrigger = AnimationTrigger::Count;
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  std::unique_ptr<BlockWorldFilter> _homeFilter;
  
  ObjectID _chargerID;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorGoHome_H__
