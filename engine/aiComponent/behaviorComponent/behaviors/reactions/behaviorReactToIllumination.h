/**
 * File: behaviorReactToIllumination.h
 *
 * Author: Humphrey Hu
 * Created: 2018-06-03
 *
 * Description: Behavior to react when the lights go out
 * 
 **/
 
 #ifndef __Victor_Behaviors_BehaviorReactToIllumination_H__
 #define __Victor_Behaviors_BehaviorReactToIllumination_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class ConditionIlluminationDetected;

class BehaviorReactToIllumination : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToIllumination( const Json::Value& config );

public:

  virtual bool WantsToBeActivatedBehavior() const override;

protected:

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override
  {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  virtual void BehaviorUpdate() override;

  struct InstanceConfig
  {
    Json::Value positiveConfig;
    Json::Value negativeConfig;
    f32 lookHeadAngle;
    f32 lookWaitTime_s;
    f32 lookTurnAmount;
    u32 numChecks;
    std::string reactionEmotion;
    std::string reactionAnimation;
  };

  enum class State
  {
    Waiting,          // Waiting to start
    Turning,          // Turning to look
    TurnedAndWaiting, // Waiting for condition
  };

  struct DynamicVariables
  {
    std::shared_ptr<ConditionIlluminationDetected> negativeCondition;
    std::shared_ptr<ConditionIlluminationDetected> positiveCondition;
    State state;
    f32 waitStartTime;
    u32 numChecksSucceeded;

    void Reset();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToTurning();
  void TransitionToTurnedAndWaiting();
  
  void RunTriggers();
};

}
}

 #endif