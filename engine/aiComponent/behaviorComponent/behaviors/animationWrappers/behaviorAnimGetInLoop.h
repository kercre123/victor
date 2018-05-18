/**
* File: behaviorAnimGetInLoop.h
*
* Author: Kevin M. Karol
* Created: 2/7/18
*
* Description: Behavior which mirrors the animation "Get In Loop" state machine
* Flow: Play GetIn animation followed by Loop animation until EndLoop condition is met
*   followed by GetOut animation
*
* Named for the animation state machine structure: https://ankiinc.atlassian.net/wiki/spaces/COZMO/pages/119668760/Animation+Statemachines+Guide
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAnimGetInLoop_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAnimGetInLoop_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorAnimGetInLoop : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAnimGetInLoop(const Json::Value& config);
  
public:
  
  virtual ~BehaviorAnimGetInLoop();
  
  virtual bool WantsToBeActivatedBehavior() const override final;

  // Notify the behavior that it should end the looping animation when it finishes
  void RequestLoopEnd() { _dVars.shouldLoopEnd = true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override  final{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates         = false;
  }
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override final;
  
  virtual void InitBehavior() override final;
  virtual void OnBehaviorActivated() override final;
  virtual void BehaviorUpdate() override final;
  virtual void AnimBehaviorUpdate() {};
  virtual void OnBehaviorDeactivated() override final;

  // For derived classes that do their own looping
  AnimationTrigger GetLoopTrigger() { return _iConfig.loopTrigger; }


private:
  enum class BehaviorStage{
    GetIn,
    Loop,
    GetOut
  };

  struct InstanceConfig {
    InstanceConfig();
    AnimationTrigger getInTrigger;
    AnimationTrigger loopTrigger;
    AnimationTrigger getOutTrigger;
    AnimationTrigger emergencyGetOutTrigger;
    float            loopInterval_s;
    bool             checkEndConditionDuringAnim;
    uint8_t          tracksToLock;

    IBEIConditionPtr endLoopCondition;
  };

  struct DynamicVariables {
    DynamicVariables();
    BehaviorStage stage;
    bool shouldLoopEnd;
    float nextLoopTime_s;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToGetIn();
  void TransitionToLoop();
  void TransitionToGetOut();

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAnimGetInLoop_H__
