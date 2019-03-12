/**
 * File: BehaviorReactToUncalibratedHeadAndLift.cpp
 *
 * Author: Arjun Menon
 * Created: 2018-08-10
 *
 * Description:
 * When the robot reports encoder readings outside
 * the range of physical expectations, calibrate these motors.
 * Note: This behavior should not run during certain power-saving
 * modes, and also only after a duration after being picked up
 * (to prevent calibration while being handled)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToUncalibratedHeadAndLift.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/animationComponent.h"
#include "engine/components/powerStateManager.h"
#include "engine/actions/basicActions.h"

#include "util/logging/DAS.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/motorTypes.h"

#include "clad/externalInterface/messageEngineToGame.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {
  
  static const std::set<BehaviorClass> kDoNotInterruptBehaviorClasses = {
    BEHAVIOR_CLASS(BlackJack),
    BEHAVIOR_CLASS(FistBump),
    BEHAVIOR_CLASS(Alexa),
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToUncalibratedHeadAndLift::BehaviorReactToUncalibratedHeadAndLift(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToUncalibratedHeadAndLift::~BehaviorReactToUncalibratedHeadAndLift()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToUncalibratedHeadAndLift::WantsToBeActivatedBehavior() const
{
  // This shouldn't interrupt pending intents. Its a rare coincidence, except apparently when transitioning from
  // onboarding to normal operation with a pending intent, when it happens constantly... could be my robot is kinda
  // borked... but this shouldn't hurt anything.
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool userIntentPending = uic.IsAnyUserIntentPending();
  const bool userIntentActive  = uic.IsAnyUserIntentActive();
  const bool triggerPending = uic.IsTriggerWordPending();
  const bool isListening = uic.IsCloudStreamOpen();
  const bool inPowerSaveMode = GetBEI().GetPowerStateManager().InPowerSaveMode();
  const bool isAnimating = GetBEI().GetAnimationComponent().IsPlayingAnimation();
  bool shouldActivate = !inPowerSaveMode &&
                        !isAnimating &&
                        !userIntentPending &&
                        !userIntentActive &&
                        !triggerPending &&
                        !isListening &&
                        (GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds() ||
                         GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds() ||
                         GetBEI().GetRobotInfo().IsHeadEncoderInvalid()   ||
                         GetBEI().GetRobotInfo().IsLiftEncoderInvalid());

  if (shouldActivate) {
    // If a calibration seems necessary, first verify that we are not in a behavior which we know can cause an encoder
    // to become invalid since manipulation by user is expected.
    const auto checkInterruptCallback = [&shouldActivate](const ICozmoBehavior& behavior)->bool {
      const bool classActive = (kDoNotInterruptBehaviorClasses.find(behavior.GetClass()) != kDoNotInterruptBehaviorClasses.end());
      if (classActive) {
        shouldActivate = false;
        return false; // stop iterating
      }
      return true; // keep iterating
    };
    const auto& behaviorIterator = GetBehaviorComp<ActiveBehaviorIterator>();
    behaviorIterator.IterateActiveCozmoBehaviorsForward( checkInterruptCallback );
  }

  return shouldActivate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::OnBehaviorActivated() 
{
  DEV_ASSERT_MSG(!GetBEI().GetPowerStateManager().InPowerSaveMode(),
                 "BehaviorReactToUncalibratedHeadAndLift.OnBehaviorActivated.InPowerSaveModeDuringCalibration", "");
  const auto& robot = GetBEI().GetRobotInfo();
  bool calibrateHead = robot.IsHeadMotorOutOfBounds() || robot.IsHeadEncoderInvalid();
  bool calibrateLift = robot.IsLiftMotorOutOfBounds() || robot.IsLiftEncoderInvalid();
  if(calibrateHead || calibrateLift) {
    LOG_INFO("BehaviorReactToUncalibratedHeadAndLift.Activated",
             "IsHeadMotorOutOfBounds %d, IsLiftMotorOutOfBounds %d, IsHeadEncoderInvalid %d, IsLiftEncoderInvalid %d",
             GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds(),
             GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds(),
             GetBEI().GetRobotInfo().IsHeadEncoderInvalid(),
             GetBEI().GetRobotInfo().IsLiftEncoderInvalid());
    
    DelegateNow(new CalibrateMotorAction(calibrateHead,
                                         calibrateLift,
                                         MotorCalibrationReason::BehaviorReactToUncalibratedHeadAndLift));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::OnBehaviorDeactivated()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUncalibratedHeadAndLift::BehaviorUpdate() 
{
}

}
}
