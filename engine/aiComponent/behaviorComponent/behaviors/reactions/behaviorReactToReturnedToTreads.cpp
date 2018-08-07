/**
 * File: behaviorReactToReturnedToTreads.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-24
 *
 * Description: Cozmo reacts to being placed back on his treads (cancels playing animations)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToReturnedToTreads.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToReturnedToTreads::BehaviorReactToReturnedToTreads(const Json::Value& config)
: ICozmoBehavior(config)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToReturnedToTreads::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToReturnedToTreads::OnBehaviorActivated()
{
  // Wait for a bit to allow pitch to correct
  DelegateIfInControl(new WaitAction(0.5f),
              &BehaviorReactToReturnedToTreads::CheckForHighPitch);


}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToReturnedToTreads::CheckForHighPitch()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  // Check for pitch exceeding a certain threshold.
  // This threshold defines how small a miscalibration can be corrected for,
  // but also inherently assumes that a well-calibrated robot won't be used on
  // a surface that is inclined this much otherwise the head will calibrate
  // every time it's placed down. So we don't want it to be too small or big.
  // 10 degrees was a selected as a conservative value, but we should keep an eye out
  // for unnecessary head calibrations.
  if (std::fabsf(robotInfo.GetPitchAngle().getDegrees()) > 10.f) {
    PRINT_NAMED_INFO("BehaviorReactToReturnedToTreads.CalibratingHead", "%f", robotInfo.GetPitchAngle().getDegrees());
    DelegateIfInControl(new CalibrateMotorAction(true, false));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToReturnedToTreads::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
