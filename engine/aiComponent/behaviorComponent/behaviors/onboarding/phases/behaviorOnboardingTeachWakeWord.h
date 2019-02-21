/**
 * File: BehaviorOnboardingTeachWakeWord.h
 *
 * Author: Sam Russell
 * Created: 2018-11-06
 *
 * Description: Maintain "eye contact" with the user while awaiting a series of wakewords. Use anims to indicate
 *              successful wakeword detections
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingTeachWakeWord__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingTeachWakeWord__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/iOnboardingPhaseWithProgress.h"

namespace Anki {
namespace Vector {

class BehaviorOnboardingTeachWakeWord : public ICozmoBehavior, public IOnboardingPhaseWithProgress
{
public: 
  virtual ~BehaviorOnboardingTeachWakeWord();

  // IOnboardingPhaseWithProgress
  virtual int GetPhaseProgressInPercent() const override;
  virtual void ResumeUponNextActivation() override {_dVars.resumeUponActivation = true;}

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingTeachWakeWord(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  void TransitionToListenForWakeWord();
  void TransitionToWaitForWakeWordGetInToFinish();
  void TransitionToReactToWakeWord();
  void TransitionToCelebrateSuccess();

  void EnableWakeWordDetection();
  void DisableWakeWordDetection();

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    ICozmoBehaviorPtr lookAtUserBehavior;
    AnimationTrigger  listenGetInAnimTrigger;
    AnimationTrigger  listenGetOutAnimTrigger;
    AnimationTrigger  celebrationAnimTrigger;
    int32_t           simulatedStreamingDuration_ms;
    uint8_t           numWakeWordsToCelebrate;
  };

  enum class TeachWakeWordState {
    ListenForWakeWord,
    WaitForWakeWordGetInToFinish,
    ReactToWakeWord,
    CelebrateSuccess
  };

  struct DynamicVariables {
    DynamicVariables();
    TeachWakeWordState state;
    bool resumeUponActivation;
    struct PersistentVars{
      PersistentVars();
      int numWakeWordDetections;
    } persistent;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingTeachWakeWord__
