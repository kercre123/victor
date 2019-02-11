/**
 * File: BehaviorOnboardingWakeUp.h
 *
 * Author: Sam Russell
 * Created: 2018-12-12
 *
 * Description: WakeUp and, if necessary, drive off the charger
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingWakeUp__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingWakeUp__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/iOnboardingPhaseWithProgress.h"

namespace Anki {
namespace Vector {

class BehaviorOnboardingWakeUp : public ICozmoBehavior, public IOnboardingPhaseWithProgress
{
public: 
  virtual ~BehaviorOnboardingWakeUp();

  // IOnboardingPhaseWithProgress
  virtual int GetPhaseProgressInPercent() const override {return _dVars.persistent.percentComplete;};
  virtual void ResumeUponNextActivation() override {_dVars.resumeUponActivation = true;}

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingWakeUp(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  void TransitionFromPhoneFace();
  void WakeUp();
  void DriveOffChargerIfNecessary();

  struct InstanceConfig {
    InstanceConfig();
  };

  enum class WakeUpState{
    NotStarted,
    TransitionFromPhoneFace,
    WakeUp,
    DriveOffCharger
  };

  struct DynamicVariables {
    DynamicVariables();
    bool resumeUponActivation;
    struct PersistentVars{
      PersistentVars();
      WakeUpState state;
      int percentComplete;
    } persistent;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingWakeUp__
