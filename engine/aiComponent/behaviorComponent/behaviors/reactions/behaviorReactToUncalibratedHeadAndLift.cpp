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


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToUncalibratedHeadAndLift.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/powerStateManager.h"
#include "engine/actions/basicActions.h"

#include "util/logging/DAS.h"
#include "clad/types/motorTypes.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

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
  const bool result = !inPowerSaveMode &&
                      (GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds() ||
                       GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds());
  return result;
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
  
  if(GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds() || GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds()) {
    DelegateNow(new CalibrateMotorAction(GetBEI().GetRobotInfo().IsHeadMotorOutOfBounds(),
                                         GetBEI().GetRobotInfo().IsLiftMotorOutOfBounds(),
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
