
/**
 * File: BehaviorOnboardingLookAtPhone1p21p2.h
 *
 * Author: Sam
 * Created: 2018-06-27
 *
 * Description: keeps the head up while displaying a look at phone animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p2/phases/behaviorOnboardingLookAtPhone1p2.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/aiComponent/behaviorComponent/onboardingMessageHandler.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone1p2::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone1p2::DynamicVariables::DynamicVariables()
{
  hasRun = false;
  receivedMessage = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone1p2::BehaviorOnboardingLookAtPhone1p2(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::InitBehavior()
{
  SubscribeToTags({GameToEngineTag::HasBleKeysResponse});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingLookAtPhone1p2::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  HandleGameToEngineEvent(event); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::HandleWhileInScopeButNotActivated(const GameToEngineEvent& event)
{
  HandleGameToEngineEvent(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::HandleGameToEngineEvent(const GameToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::HasBleKeysResponse:
    {
      _hasBleKeys = event.GetData().Get_HasBleKeysResponse().hasBleKeys;
      MoveHeadUp();

      break;
    }
      
    default:
      break;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::OnBehaviorActivated()
{
  SmartDisableKeepFaceAlive();
  bool hasRun = _dVars.hasRun;
  _dVars = DynamicVariables();
  _dVars.hasRun = true;

  // since this behavior runs on and off charger without any user facing battery alerts, make sure it's in low power
  // mode to avoid overheating the battery. We should be able to still receive app messages during this time to wake
  // up (which ends low power mode).
  SmartRequestPowerSaveMode();

  GetBehaviorComp<OnboardingMessageHandler>().RequestBleSessions();

  // if the app requests we restart onboarding in the middle of something else, make sure the lift is down
  auto* moveLiftAction = new MoveLiftToHeightAction( MoveLiftToHeightAction::Preset::LOW_DOCK );
  DelegateIfInControl( moveLiftAction, [this, hasRun]() {
    if( hasRun ) {
      // start with the loop action, which has a delayed head keyframe in the UP position, instead
      // of MoveHeadUp, which has an initial keyframe in the DOWN position, to avoid a head snap
      if(_hasBleKeys) {
        RunLoopAction();
      }
    } else {
      MoveHeadUp();
    }
  });

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::BehaviorUpdate()
{
  if( IsActivated() && !IsControlDelegated() ) {
    // this can happen if the robot cancels all actions (like when it detect that it's falling)
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::MoveHeadUp()
{
  if(_hasBleKeys) {
    GetBehaviorComp<OnboardingMessageHandler>().ShowUrlFace(false);
    auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneUp };
    action->SetRenderInEyeHue( false );
    DelegateIfInControl(action, [this](const ActionResult& res){
      RunLoopAction();
    });
  } else {
    MoveHeadToAngleAction* action = new MoveHeadToAngleAction(MAX_HEAD_ANGLE);
    DelegateIfInControl(action);
    GetBehaviorComp<OnboardingMessageHandler>().ShowUrlFace(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone1p2::RunLoopAction()
{
  auto* loopAction = new ReselectingLoopAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneLoop };
  loopAction->SetRenderInEyeHue( false );
  DelegateIfInControl( loopAction ); // loop forever, waiting for a message
}

} // namespace Vector
} // namespace Anki
