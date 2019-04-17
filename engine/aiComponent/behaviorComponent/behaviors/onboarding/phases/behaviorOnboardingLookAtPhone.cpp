
/**
 * File: BehaviorOnboardingLookAtPhone.cpp
 *
 * Author: Sam
 * Created: 2018-06-27
 *
 * Description: keeps the head up while displaying a look at phone animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/behaviorOnboardingLookAtPhone.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/onboardingMessageHandler.h"
#include "engine/components/animationComponent.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/cozmoContext.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::DynamicVariables::DynamicVariables()
{
  hasRun = false;
  allowTooHotToChargeCheck = false;
  displayingTooHotToCharge = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::BehaviorOnboardingLookAtPhone(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::InitBehavior()
{
  SubscribeToTags({GameToEngineTag::HasBleKeysResponse});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingLookAtPhone::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  HandleGameToEngineEvent(event); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::HandleWhileInScopeButNotActivated(const GameToEngineEvent& event)
{
  HandleGameToEngineEvent(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::HandleGameToEngineEvent(const GameToEngineEvent& event)
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
void BehaviorOnboardingLookAtPhone::BehaviorUpdate()
{
  if( IsActivated()) {
    CheckIfTooHotToCharge();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadUp()
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
    DelegateNow(action);
    GetBehaviorComp<OnboardingMessageHandler>().ShowUrlFace(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::CheckIfTooHotToCharge()
{
  if (!_dVars.allowTooHotToChargeCheck) {
    return;
  }

  // If battery is too hot to charge then display too hot image
  auto& robotInfo = GetBEI().GetRobotInfo();
  const auto& battComp = robotInfo.GetBatteryComponent();  
  if (battComp.IsChargingStalledBecauseTooHot()) {
    if (!_dVars.displayingTooHotToCharge) {
      CancelDelegates(); // Cancel the looping phone anim if running

      LOG_INFO("BehaviorOnboardingLookAtPhone.CheckIfTooHotToCharge.BatteryIsTooHot", "");
      const auto& dataPlatform = robotInfo.GetContext()->GetDataPlatform();
      static const std::string kTooHotToChargeImg = "config/sprites/independentSprites/battery_overheated.png";
      const std::string imgPath = dataPlatform->pathToResource(Anki::Util::Data::Scope::Resources, 
                                                               kTooHotToChargeImg);
      Vision::ImageRGB img;
      img.Load(imgPath);
      auto& animComp = GetBEI().GetAnimationComponent();
      animComp.DisplayFaceImage(img, 0, true);

      _dVars.displayingTooHotToCharge = true;
    }
  } else if (_dVars.displayingTooHotToCharge) {
    // Loop phone anim now that battery is no longer too hot to charge
    _dVars.displayingTooHotToCharge = false;
    RunLoopAction();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::RunLoopAction()
{
  _dVars.allowTooHotToChargeCheck = true;
  CheckIfTooHotToCharge();

  // Loop phone animation
  if (!_dVars.displayingTooHotToCharge) {
    auto* loopAction = new ReselectingLoopAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneLoop };
    loopAction->SetRenderInEyeHue( false );
    DelegateIfInControl( loopAction ); // loop forever, waiting for a message
  }
}

} // namespace Vector
} // namespace Anki
