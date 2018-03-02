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
  
  virtual bool WantsToBeActivatedBehavior() const override;

  // Notify the behavior that it should end the looping animation when it finishes
  void RequestLoopEnd() { _dVars.shouldLoopEnd = true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates         = false;
  }
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;


private:
  enum class BehaviorStage{
    GetIn,
    Loop,
    GetOut
  };

  struct InstanceConfig {
    AnimationTrigger getInTrigger                = AnimationTrigger::Count;
    AnimationTrigger loopTrigger                 = AnimationTrigger::Count;
    AnimationTrigger getOutTrigger               = AnimationTrigger::Count;
    AnimationTrigger emergencyGetOutTrigger      = AnimationTrigger::Count;
    float            loopInterval_s              = 0;
    bool             checkEndConditionDuringAnim = true;
    IBEIConditionPtr endLoopCondition;
  };

  struct DynamicVariables {
    BehaviorStage stage  = BehaviorStage::GetIn;
    bool shouldLoopEnd   = false;
    float nextLoopTime_s = 0;
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
