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
namespace Vector {

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
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
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

  struct InstanceConfig{
    InstanceConfig();
    float maxPounceDist;
    float minGroundAreaForPounce;
    float pounceTimeout_s;
    IBEIConditionPtr inRangeCondition;
  };

  struct DynamicVariables{
    DynamicVariables();
    float prePouncePitch;
    bool motionObserved;
    float pounceAtTime_s;
    PounceState pounceState;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;


  void TransitionToResultAnim();
  bool IsFingerCaught();

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  void TransitionToPounce();

};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPounceWithProx_H__
