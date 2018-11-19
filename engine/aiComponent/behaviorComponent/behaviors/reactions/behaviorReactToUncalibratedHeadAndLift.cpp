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
#include "engine/components/powerStateManager.h"
#include "engine/actions/basicActions.h"

#include "util/logging/DAS.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/motorTypes.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

namespace {
  static const std::set<BehaviorID> kDoNotInterruptBehaviors = {
    BEHAVIOR_ID(FistBump),
    BEHAVIOR_ID(FistBumpVoiceCommand),
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
  const bool inPowerSaveMode = GetBEI().GetPowerStateManager().InPowerSaveMode();
  bool shouldActivate = !inPowerSaveMode &&
                        (GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds() ||
                         GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds() ||
                         GetBEI().GetRobotInfo().IsHeadEncoderInvalid()   ||
                         GetBEI().GetRobotInfo().IsLiftEncoderInvalid());

  if (shouldActivate) {
    // If a calibration seems necessary, first verify that we're not in the FistBump behavior which we know
    // can cause the lift encoder to become invalid since manipulation by user is expected.
    const auto checkInterruptCallback = [&shouldActivate](const ICozmoBehavior& behavior)->bool {
      auto got = kDoNotInterruptBehaviors.find(behavior.GetID());
      if(got != kDoNotInterruptBehaviors.end()) {
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
    DelegateNow(new CalibrateMotorAction(calibrateHead,
                                         calibrateLift,
                                         EnumToString(MotorCalibrationReason::BehaviorReactToUncalibratedHeadAndLift)));
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
