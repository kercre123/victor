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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "clad/externalInterface/messageGameToEngine.h"

#include "coretech/vision/engine/image.h"
#include "opencv2/highgui/highgui.hpp"
#include "osState/osState.h"

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
  receivedMessage = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingLookAtPhone::BehaviorOnboardingLookAtPhone(const Json::Value& config)
 : ICozmoBehavior(config)
{
  SubscribeToTags({
    ExternalInterface::MessageGameToEngineTag::SendBLEConnectionStatus
  });
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
  const auto persistent = _dVars.persistent;
  bool hasRun = _dVars.hasRun;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  _dVars.hasRun = true;
  
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
  auto& robotInfo = GetBEI().GetRobotInfo();
  if( IsActivated() && 
      !IsControlDelegated() && 
      !robotInfo.IsPickedUp() &&
      robotInfo.IsHeadCalibrated() ) {
    // this can happen if the robot cancels all actions (like when it detect that it's falling)
    PRINT_NAMED_WARNING("BehaviorOnboardingLookAtPhone.Update.MoveHead", "");
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
  IActionRunner* action = nullptr;
  if (!_dVars.persistent.hasBLEClient) {
    action = new MoveHeadToAngleAction(MAX_HEAD_ANGLE);
  } else {
    auto* animAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneUp };
    animAction->SetRenderInEyeHue( false );
    action = animAction;
  }

  DelegateIfInControl(action, [this](const ActionResult& res){
    RunLoopAction();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::RunLoopAction()
{
  if (DisplayURLScreen()) {
    DelegateIfInControl(new HangAction());
  } else {
    auto* loopAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneLoop, 0 };
    loopAction->SetRenderInEyeHue( false );
    DelegateIfInControl( loopAction ); // loop forever, waiting for a message
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::MoveHeadDown()
{
  IActionRunner* action = nullptr;
  if (DisplayURLScreen()) {
    action = new MoveHeadToAngleAction(MIN_HEAD_ANGLE);
  } else {
    auto* animAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingLookAtPhoneDown };
    animAction->SetRenderInEyeHue( false );
    action = animAction;
  }
 
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Displays the URL screen only if no BLE client is connected
bool BehaviorOnboardingLookAtPhone::DisplayURLScreen()
{
  if (!_dVars.persistent.hasBLEClient) {
    
    // Copied URL face drawing logic from ConnectionFlow::DrawStartPairingScreen()
    const f32 kRobotNameScale = 0.6f;
    const std::string kURL = "anki.com/v";
    const ColorRGBA   kColor(0.9f, 0.9f, 0.9f, 1.f);

    // Robot name will be empty until switchboard has set the property
    std::string robotName = OSState::getInstance()->GetRobotName();
    if(robotName == "")
    {
      PRINT_NAMED_WARNING("BehaviorOnboardingLookAtPhone.DisplayURLScreen.NoRobotName", "");
      return false;
    }

    Vision::ImageRGB565 img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    img.FillWith(Vision::PixelRGB565(0, 0, 0));

    img.DrawTextCenteredHorizontally(robotName, CV_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

    cv::Size textSize;
    float scale = 0;
    Vision::Image::MakeTextFillImageWidth(kURL, CV_FONT_NORMAL, 1, img.GetNumCols(), textSize, scale);
    img.DrawTextCenteredHorizontally(kURL, CV_FONT_NORMAL, scale, 1, kColor, (FACE_DISPLAY_HEIGHT + textSize.height)/2, true);

    GetBEI().GetAnimationComponent().DisplayFaceImage(img, 0, true);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingLookAtPhone::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  const auto& eventData = event.GetData();
  switch(eventData.GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::SendBLEConnectionStatus:
    {
      _dVars.persistent.hasBLEClient = eventData.Get_SendBLEConnectionStatus().connected;
      PRINT_CH_INFO("Behaviors", "BehaviorOnboardingLookAtPhone.AlwaysHandleInScope.BLEConnectionStatus",
                    "%d", _dVars.persistent.hasBLEClient);

      if (IsActivated()) {
        CancelDelegates(false);
      }
      break;
    }
    default:
    {
      PRINT_CH_INFO("Behaviors", "BehaviorOnboardingLookAtPhone.AlwaysHandle.InvalidTag",
                    "Received unexpected event with tag %hhu.", eventData.GetTag());
      break;
    }
  }
}

}
}
