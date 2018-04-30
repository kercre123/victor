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

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPlacedOnSlope::BehaviorReactToPlacedOnSlope(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::WantsToBeActivatedBehavior() const
{
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
    PRINT_NAMED_INFO("BehaviorReactToPlacedOnSlope.CalibratingHead", "%f", robotInfo.GetPitchAngle().getDegrees());
    DelegateIfInControl(new CalibrateMotorAction(true, false));
    _endedOnInclineLastTime = false;
  } else {
    // Play the animation then check if we're still on a slope or if we were perched on something:

    AnimationTrigger reactionAnim = AnimationTrigger::ANTICIPATED_ReactToPerchedOnBlock;

    DelegateIfInControl(new TriggerAnimationAction(reactionAnim),
                        &BehaviorReactToPlacedOnSlope::CheckPitch);
  }

  _lastBehaviorTime = now;

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::CheckPitch()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // Was the robot on an inclined surface or was the lift simply perched on something?
  _endedOnInclineLastTime = (robotInfo.GetPitchAngle().getDegrees() > 10.0f);
}


} // namespace Cozmo
} // namespace Anki
