/**
 * File: behaviorPounceWithProx.h
 *
 * Author: Kevin M. Karol
 * Created: 2017-12-6
 *
 * Description: Test out prox sensor pounce vocabulary
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPounceWithProx_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPounceWithProx_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorPounceWithProx : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPounceWithProx(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  enum class PounceState{
    WaitForMotion,
    PounceOnMotion,
    ReactToPounce
  };

  struct {
    float maxPounceDist = 120.0f;
    float minGroundAreaForPounce = 0.01f;
    float pounceTimeout_s = 5;
    IBEIConditionPtr inRangeCondition;
  } _instanceParams;

  struct LifetimeParams{
    float prePouncePitch = 0.0f;
    bool motionObserved = false;
    float pounceAtTime_s = std::numeric_limits<float>::max();
    PounceState pounceState = PounceState::WaitForMotion;
  };
  LifetimeParams _lifetimeParams;


  void TransitionToResultAnim();
  bool IsFingerCaught();

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  void TransitionToPounce();

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPounceWithProx_H__
