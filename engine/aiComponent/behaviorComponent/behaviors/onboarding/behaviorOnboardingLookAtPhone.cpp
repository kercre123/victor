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
#include "proto/external_interface/shared.pb.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::BehaviorOnboardingLookAtPhone(const Json::Value& config)
 : ICozmoBehavior(config)
{
  SetupConsoleFuncs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::InitBehavior()
{
  SubscribeToAppTags({
    AppToEngineTag::kOnboardingConnectionComplete,
  });
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
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::OnBehaviorActivated() 
{
  MoveHeadUp();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::HandleWhileActivated(const AppToEngineEvent& event)
{
  if( event.GetData().GetTag() == AppToEngineTag::kOnboardingConnectionComplete ) {
    MoveHeadDown();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadUp()
{
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneUp };
  DelegateIfInControl(action, [this](const ActionResult& res){
    auto* loopAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneLoop, 0 };
    DelegateIfInControl( loopAction ); // loop forever, waiting for a message
  });
}
  
void BehaviorOnboardingLookAtPhone::MoveHeadDown()
{
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneDown };
  DelegateNow( action ); // and then the behavior ends
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::SetupConsoleFuncs()
{
  if( ANKI_DEV_CHEATS ) {
    if( _iConfig.consoleFuncs.empty() ) {
      // console func to mimic the app sending OnboardingConnectionComplete
      auto func = [this](ConsoleFunctionContextRef context) {
        MoveHeadDown();
      };
      _iConfig.consoleFuncs.emplace_front( "EndPhoneIcon", std::move(func), "Onboarding", "" );
    }
  }
}

}
}
