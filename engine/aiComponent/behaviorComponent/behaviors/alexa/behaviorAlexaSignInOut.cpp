/**
 * File: behaviorAlexaSignInOut.cpp
 *
 * Author: ross
 * Created: 2018-12-02
 *
 * Description: handles commands to sign in and out of amazon
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/alexa/behaviorAlexaSignInOut.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/alexaComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Vector {
  
namespace {
  // wait this long before signing in
  const float kSignInDelay_s = 1.0f;
  // wait this long for the face info screen to kick in
  const float kExitDelay_s = 3.0f;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexaSignInOut::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexaSignInOut::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexaSignInOut::BehaviorAlexaSignInOut(const Json::Value& config)
 : ICozmoBehavior(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::InitBehavior()
{
  _iConfig.lookAtFaceOrUpBehavior = FindBehavior("LookAtFaceOrUpInternal");
  _iConfig.wakeWordBehaviors.AddBehavior( GetBEI().GetBehaviorContainer(), BEHAVIOR_CLASS(ReactToVoiceCommand) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAlexaSignInOut::WantsToBeActivatedBehavior() const
{
  const auto& alexaComp = GetAIComp<AlexaComponent>();
  const bool signInPending = alexaComp.IsSignInPending();
  const bool signOutPending = alexaComp.IsSignOutPending();
  // don't run if the wake word behavior is active, since BehaviorAlexaSignInOut could be higher priority, but
  // we want to give the wake word behavior time to finish
  const bool wakeWordComplete = !_iConfig.wakeWordBehaviors.AreBehaviorsActivated();
  
  return (signInPending || signOutPending) && wakeWordComplete;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  
  // let AlexaComponent handle it in these cases:
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.lookAtFaceOrUpBehavior ) {
    delegates.insert( _iConfig.lookAtFaceOrUpBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  auto& alexaComp = GetAIComp<AlexaComponent>();
  const bool signInPending = alexaComp.IsSignInPending();
  const bool signOutPending = alexaComp.IsSignOutPending();
  DEV_ASSERT( signInPending != signOutPending, "BehaviorAlexaSignInOut.OnBehaviorActivated.TwoCommands" );
  _dVars.signingIn = signInPending;
  
  // stop AlexaComponent for acting on it for now
  alexaComp.ClaimRequest();
  
  auto callback = [this](){
    SignInOrOut();
    if( _dVars.signingIn ) {
      // wait for face info screen to kick in
      _dVars.exitTimeout_s = kExitDelay_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    } else {
      PlaySignOutAnimAndExit();
    }
  };
  // if on charger, this behavior will only move the head
  if( _iConfig.lookAtFaceOrUpBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.lookAtFaceOrUpBehavior.get(), callback );
  } else {
    callback();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::OnBehaviorDeactivated()
{
  // make sure sign in/out is handled if this behavior gets interrupted. won't do anything if already handled
  SignInOrOut();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  if( (_dVars.exitTimeout_s >= 0.0f)
      && (_dVars.exitTimeout_s <= BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) )
  {
    CancelSelf();
  }
  
  if( _dVars.signingIn && (GetActivatedDuration() >= kSignInDelay_s) ) {
    // it takes a second or so for the server to respond with a code, so start that process now.
    // does nothing if already handled
    SignInOrOut();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::SignInOrOut()
{
  if( !_dVars.handled ) {
    _dVars.handled = true;
    auto& alexaComp = GetAIComp<AlexaComponent>();
    if( _dVars.signingIn ) {
      alexaComp.SignIn();
    } else {
      alexaComp.SignOut();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexaSignInOut::PlaySignOutAnimAndExit()
{
  // hack: until there's a new stub re-use blackjack animation but with audio track locked
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaSignOut, 1, true, (u8)AnimTrackFlag::AUDIO_TRACK };
  DelegateIfInControl( action, [this](const ActionResult& res){
    CancelSelf();
  });
}

}
}
