/**
 * File: BehaviorOnboardingWakeUp.cpp
 *
 * Author: Sam Russell
 * Created: 2018-12-12
 *
 * Description: WakeUp and, if necessary, drive off the charger
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/behaviorOnboardingWakeUp.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingWakeUp::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingWakeUp::DynamicVariables::DynamicVariables()
: resumeUponActivation(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingWakeUp::DynamicVariables::PersistentVars::PersistentVars()
: state(WakeUpState::NotStarted)
, percentComplete(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingWakeUp::BehaviorOnboardingWakeUp(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingWakeUp::~BehaviorOnboardingWakeUp()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingWakeUp::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::OnBehaviorActivated() 
{
  // reset dynamic variables, accounting for persistence when necessary
  if( _dVars.resumeUponActivation ){
    auto resumeVars = _dVars.persistent;
    _dVars = DynamicVariables();
    _dVars.persistent = resumeVars;
  } else {
    _dVars = DynamicVariables();
  }

  if( WakeUpState::NotStarted == _dVars.persistent.state ){
    TransitionFromPhoneFace();
  } else if( WakeUpState::TransitionFromPhoneFace == _dVars.persistent.state ){
    WakeUp();
  } else {
    DriveOffChargerIfNecessary();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::TransitionFromPhoneFace()
{
  _dVars.persistent.state = WakeUpState::TransitionFromPhoneFace;
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::OnboardingLookAtPhoneDown),
                      &BehaviorOnboardingWakeUp::WakeUp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::WakeUp()
{
  _dVars.persistent.percentComplete = 33;
  _dVars.persistent.state = WakeUpState::WakeUp;
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::OnboardingWakeUp),
                      &BehaviorOnboardingWakeUp::DriveOffChargerIfNecessary);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingWakeUp::DriveOffChargerIfNecessary()
{
  _dVars.persistent.percentComplete = 66;
  _dVars.persistent.state = WakeUpState::DriveOffCharger;

  auto driveOffChargerCallback = [this](){
      _dVars.persistent.percentComplete = 100;
      CancelSelf();
  };

  if( GetBEI().GetRobotInfo().IsOnChargerPlatform() ){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::OnboardingDriveOffCharger), 
                        driveOffChargerCallback);
  }
}

}
}
