/**
 * File: behaviorReactToPlacedOnSlope.cpp
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {
  
namespace {
  // Bit flags for each of the front-facing cliff sensors:
  const u8 FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const u8 FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const u8 FRONT = FL|FR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPlacedOnSlope::BehaviorReactToPlacedOnSlope(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::WantsToBeActivatedBehavior() const
{
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();
  if (cliffComp.IsCliffDetected()) {
    // When placed on a slope, only the front-facing cliff sensors should be detecting cliffs,
    // since the animation that plays from this behavior drives the robot backwards.
    const auto cliffFlags = cliffComp.GetCliffDetectedFlags();
    return AreCliffDetectedFlagsValid(cliffFlags);
  }
  // When the robot is simply placed on a ramp, no cliffs are detected, or if the robot is
  // perched too close to the ground, the cliff sensors may not detect any cliffs at all.
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::OnBehaviorActivated()
{
  const double now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasBehaviorRunRecently = (now - _lastBehaviorTime < 10.0);

  // Double check that we should play the animation or recalibrate:
  if (hasBehaviorRunRecently && _endedOnInclineLastTime) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    // Don't run the animation. Instead, run a motor cal since his head may be out of calibration.
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPlacedOnSlope.CalibratingHead", "%f", robotInfo.GetPitchAngle().getDegrees());
    DelegateIfInControl(new CalibrateMotorAction(true, false, MotorCalibrationReason::BehaviorReactToPlacedOnSlope));
    _endedOnInclineLastTime = false;
  } else {
    // Play the animation then check if we're still on a slope or if we were perched on something:

    AnimationTrigger reactionAnim = AnimationTrigger::ReactToPerchedOnBlock;

    DelegateIfInControl(new TriggerAnimationAction(reactionAnim),
                        &BehaviorReactToPlacedOnSlope::CheckPitch);
  }

  _lastBehaviorTime = now;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();
  const auto cliffFlags = cliffComp.GetCliffDetectedFlags();
  // The animation to drive backwards to "un-perch" the robot might cause the robot to drive onto a cliff behind it.
  // Cancel the behavior if a cliff is suddenly detected by either of the rear cliff sensors.
  if (cliffComp.IsCliffDetected() && !AreCliffDetectedFlagsValid(cliffFlags)) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPlacedOnSlope.CancelSelf", "Invalid cliffs detected: 0x%X", cliffFlags);
    CancelSelf();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::AreCliffDetectedFlagsValid(const u8 cliffDetectedFlags) const
{
  return (cliffDetectedFlags & ~FRONT) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::CheckPitch()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // Was the robot on an inclined surface or was the lift simply perched on something?
  _endedOnInclineLastTime = (robotInfo.GetPitchAngle().getDegrees() > 10.0f);
}

} // namespace Vector
} // namespace Anki
