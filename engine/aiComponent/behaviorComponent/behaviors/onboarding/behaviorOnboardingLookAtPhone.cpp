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

#include "anki/cozmo/shared/cozmoConfig.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const float kHeadDownSpeed_rps = 3.0f;
}
  

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
  const bool up = true;
  MoveHeadWithAnimation( up );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::HandleWhileActivated(const AppToEngineEvent& event)
{
  if( event.GetData().oneof_message_type_case() == AppToEngineTag::kOnboardingConnectionComplete ) {
    const bool headUp = false;
    MoveHeadWithAnimation( headUp );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadWithAnimation( bool headUp )
{
  // TODO (VIC-4158) use actual animation group for at least the anim_, and possibly even two animations to mimic the two
  // uses of this method
  auto* moveHead = new MoveHeadToAngleAction( headUp ? MAX_HEAD_ANGLE : MIN_HEAD_ANGLE );
  auto* animAction = new PlayAnimationAction( "anim_pairing_icon_awaitingapp", 0 );
  if( !headUp ) {
    moveHead->SetMaxSpeed( kHeadDownSpeed_rps );
  }
  auto* compound = new CompoundActionParallel({ moveHead, animAction });
  // this behavior should end once the head goes back down
  compound->SetShouldEndWhenFirstActionCompletes( !headUp );
  DelegateNow( compound, [](){} );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::SetupConsoleFuncs()
{
  if( ANKI_DEV_CHEATS ) {
    if( _iConfig.consoleFuncs.empty() ) {
      // console func to mimic the app sending OnboardingConnectionComplete
      auto func = [this](ConsoleFunctionContextRef context) {
        const bool headUp = false;
        MoveHeadWithAnimation( headUp );
      };
      _iConfig.consoleFuncs.emplace_front( "EndPhoneIcon", std::move(func), "Onboarding", "" );
    }
  }
}

}
}
