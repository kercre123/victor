/**
 * File: behaviorReactToFrustration.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-09
 *
 * Description: Behavior to react when the robot gets really frustrated (e.g. because he is failing actions)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToFrustration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToFrustration_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class IFrustrationListener;
  
class BehaviorReactToFrustration : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToFrustration(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  
  virtual void AddListener(ISubtaskListener* listener) override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:
  void TransitionToReaction();
  void AnimationComplete();
  
  void LoadJson(const Json::Value& config);
  
  f32 _minDistanceToDrive_mm = 0;
  f32 _maxDistanceToDrive_mm = 0;
  f32 _randomDriveAngleMin_deg = 0;
  f32 _randomDriveAngleMax_deg = 0;
  AnimationTrigger _animToPlay = AnimationTrigger::Count;
  std::string _finalEmotionEvent;
  
  std::set<ISubtaskListener*> _frustrationListeners;

};


} // namespace Cozmo
} // namespace Anki

#endif
