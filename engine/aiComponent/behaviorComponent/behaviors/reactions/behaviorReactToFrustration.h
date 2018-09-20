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
#include <vector>

namespace Anki {
namespace Vector {

enum class AnimationTrigger : int32_t;
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
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:
  void TransitionToReaction();
  void AnimationComplete();
  
  void LoadJson(const Json::Value& config);
  
  struct InstanceConfig {
    InstanceConfig();
    f32 minDistanceToDrive_mm;
    f32 maxDistanceToDrive_mm;
    f32 randomDriveAngleMin_deg;
    f32 randomDriveAngleMax_deg;
    AnimationTrigger animToPlay;
    std::string finalEmotionEvent;
  
    std::set<ISubtaskListener*> frustrationListeners;
  };
  InstanceConfig _iConfig;

};


} // namespace Vector
} // namespace Anki

#endif
