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
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPounceWithProx(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override { return true;}

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
    BackupToIdealDistance,
    ApproachObject,
    WaitForMotion,
    PounceOnMotion,
    ReactToPounce
  } _pounceState = PounceState::BackupToIdealDistance;

  ICozmoBehaviorPtr _backupBehavior;
  ICozmoBehaviorPtr _approachBehavior;


  float _prePouncePitch = 0.0f;

  void TransitionToResultAnim();
  bool IsFingerCaught();


  // vision work
  float _maxPounceDist = 120.0f;
  int16_t _observedX = 0;
  int16_t _observedY = 0;
  bool _motionObserved = false;
  float _minGroundAreaForPounce = 0.01f;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  void TransitionToPounce();

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPounceWithProx_H__
