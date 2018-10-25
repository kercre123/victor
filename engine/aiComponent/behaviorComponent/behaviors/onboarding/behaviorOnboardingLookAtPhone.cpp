/**
 * File: behaviorOnboardingLookAtPhone.cpp
 *
 * Author: ross
 * Created: 2018-06-27
 *
 * Description: keeps the head up while displaying a look at phone animation, lowers the head, then ends
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingLookAtPhone.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::DynamicVariables::DynamicVariables()
{
  receivedMessage = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::BehaviorOnboardingLookAtPhone(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::InitBehavior()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingLookAtPhone::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::OnBehaviorActivated() 
{
  SmartDisableKeepFaceAlive();
  bool hasRun = _dVars.hasRun;
  _dVars = DynamicVariables();
  _dVars.hasRun = true;
  
  // since this behavior runs on and off charger without any user facing battery alerts, make sure it's in low power
  // mode to avoid overheating the battery. We should be able to still receive app messages during this time to wake
  // up (which ends low power mode).
  SmartRequestPowerSaveMode();
  
  // if the app requests we restart onboarding in the middle of something else, make sure the lift is down
  auto* moveLiftAction = new MoveLiftToHeightAction( MoveLiftToHeightAction::Preset::LOW_DOCK );
  DelegateIfInControl( moveLiftAction, [this, hasRun]() {
    if( hasRun ) {
      // start with the loop action, which has a delayed head keyframe in the UP position, instead
      // of MoveHeadUp, which has an initial keyframe in the DOWN position, to avoid a head snap
      RunLoopAction();
    } else {
      MoveHeadUp();
    }
  });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::BehaviorUpdate()
{
  if( IsActivated() && !IsControlDelegated() ) {
    // this can happen if the robot cancels all actions (like when it detect that it's falling)
    if( !_dVars.receivedMessage ) {
      MoveHeadUp();
    } else {
      MoveHeadDown();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadUp()
{
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneUp };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl(action, [this](const ActionResult& res){
    RunLoopAction();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::RunLoopAction()
{
  auto* loopAction = new ReselectingLoopAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneLoop };
  loopAction->SetRenderInEyeHue( false );
  DelegateIfInControl( loopAction ); // loop forever, waiting for a message
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadDown()
{
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneDown };
  action->SetRenderInEyeHue( false );
  DelegateNow( action, [this](const ActionResult& res){
    CancelSelf();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::ContinueReceived()
{
  if( ANKI_VERIFY( IsActivated(), "BehaviorOnboardingLookAtPhone.ContinueReceived.NotActivated", "" ) ) {
    CancelDelegates(false);
    _dVars.receivedMessage = true;
    MoveHeadDown();
  }
}

}
}
