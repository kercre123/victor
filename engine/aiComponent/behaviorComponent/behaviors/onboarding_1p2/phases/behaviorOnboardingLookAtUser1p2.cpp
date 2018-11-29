/**
 * File: BehaviorOnboardingLookAtUser1p2.cpp
 *
 * Author: Sam Russell
 * Created: 2018-11-05
 *
 * Description: Onboarding "Death Stare" implementation that handles On/Off charger state. Behavior does not return
 *              on its own, and expects to be interrupted by a timeout or new onboarding state from the App
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p2/phases/behaviorOnboardingLookAtUser1p2.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/robot.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
  _dVars.state = LookAtUserState::s; \
  SetDebugStateName(#s);\
} while(0);

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtUser1p2::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtUser1p2::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtUser1p2::BehaviorOnboardingLookAtUser1p2(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtUser1p2::~BehaviorOnboardingLookAtUser1p2()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.behaviorLookAtUser = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingLookAtUser) );
  _iConfig.behaviorLookAtUserOnCharger = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingLookAtUserOnCharger) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingLookAtUser1p2::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.behaviorLookAtUser.get() );
  delegates.insert( _iConfig.behaviorLookAtUserOnCharger.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  TransitionToLookAtUser();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }

  auto& robotInfo = GetBEI().GetRobotInfo();
  const bool onCharger = robotInfo.IsOnChargerPlatform();
  const bool inOnChargerState = ( _dVars.state == LookAtUserState::LookAtUserOnCharger );
  const bool onTreads = ( robotInfo.GetOffTreadsState() == OffTreadsState::OnTreads );
  const bool inOffTreadsState = ( _dVars.state == LookAtUserState::WaitForPutDown );

  if( !inOffTreadsState && !onTreads ) {
    TransitionToWaitingForPutDown();
  } else if( onTreads && inOnChargerState != onCharger ) {
    TransitionToLookAtUser();
  }

  if( !IsControlDelegated() ){
    // Shouldn't happen, but just in case
    LOG_ERROR( "BehaviorOnboardingLookAtUser1p2.UnexpectedNotDelegated",
               "Control should always be delegated. Investigate");
    TransitionToLookAtUser();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::TransitionToLookAtUser()
{
  CancelDelegates( false );

  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( onCharger ){
    _dVars.lastDelegate = _iConfig.behaviorLookAtUserOnCharger;
    SET_STATE( LookAtUserOnCharger );
  }
  else{
    _dVars.lastDelegate = _iConfig.behaviorLookAtUser;
    SET_STATE( LookAtUserOffCharger );
  }

  if( _dVars.lastDelegate->WantsToBeActivated() ){
    // If we exit the behavior, just re-evaluate and renter
    DelegateIfInControl( _dVars.lastDelegate.get(),
      [this](){
        LOG_INFO( "BehaviorOnboardingLookAtUser1p2.Reselect",
                  "Behavior %s returned control. Reselecting delegate",
                  _dVars.lastDelegate->GetDebugLabel().c_str());
        TransitionToLookAtUser();
      });
  }
  else{
    LOG_ERROR( "BehaviorOnboardingLookAtUser1p2.DelegationError",
               "Behavior %s did not WantToBeActivated",
               _dVars.lastDelegate->GetDebugLabel().c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtUser1p2::TransitionToWaitingForPutDown()
{
  CancelDelegates( false );
  SET_STATE( WaitForPutDown );

  auto waitForOnTreads = [](Robot& robot) {
    return robot.GetOffTreadsState() == OffTreadsState::OnTreads;
  };

  auto* action = new WaitForLambdaAction{ waitForOnTreads, std::numeric_limits<float>::max() };
  DelegateIfInControl( action, [this](){
    if( GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads ) {
      // still not on treads... was likely falling again, causing this action to abort
      TransitionToWaitingForPutDown();
    } else {
      TransitionToLookAtUser();
    }
  });
}

}// namespace Vector
}// namespace Anki
