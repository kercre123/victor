/**
 * File: BehaviorOnboardingEmulate1p0WaitForVC.cpp
 *
 * Author: Sam Russell
 * Created: 2018-12-10
 *
 * Description: Reimplementing 1p0 onboarding in the new framework: sit still and await a VC, exit onboarding if one is recieved.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/behaviorOnboardingEmulate1p0WaitForVC.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingEmulate1p0WaitForVC::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingEmulate1p0WaitForVC::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingEmulate1p0WaitForVC::BehaviorOnboardingEmulate1p0WaitForVC(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingEmulate1p0WaitForVC::~BehaviorOnboardingEmulate1p0WaitForVC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingEmulate1p0WaitForVC::InitBehavior()
{
  auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.lookAtUserBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingLookAtUser) );
  _iConfig.reactToTriggerWordBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(TriggerWordDetected) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingEmulate1p0WaitForVC::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingEmulate1p0WaitForVC::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingEmulate1p0WaitForVC::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.lookAtUserBehavior.get());
  delegates.insert(_iConfig.reactToTriggerWordBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingEmulate1p0WaitForVC::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  DelegateIfInControl(_iConfig.lookAtUserBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingEmulate1p0WaitForVC::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  if( !_iConfig.reactToTriggerWordBehavior->IsActivated() && 
      _iConfig.reactToTriggerWordBehavior->WantsToBeActivated() ){
    CancelDelegates(false);
    DelegateIfInControl(_iConfig.reactToTriggerWordBehavior.get(), [this](){
      auto& uic = GetBehaviorComp<UserIntentComponent>();
      if( uic.IsAnyUserIntentPending() ){
        CancelSelf();
      } else {
        DelegateIfInControl(_iConfig.lookAtUserBehavior.get());
      }
    });
  }

}

}
}
