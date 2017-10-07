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
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToReturnedToTreads::BehaviorReactToReturnedToTreads(const Json::Value& config)
: ICozmoBehavior(config)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToReturnedToTreads::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToReturnedToTreads::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Wait for a bit to allow pitch to correct
  DelegateIfInControl(new WaitAction(robot, 0.5f),
              &BehaviorReactToReturnedToTreads::CheckForHighPitch);
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToReturnedToTreads::CheckForHighPitch(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Check for pitch exceeding a certain threshold.
  // This threshold defines how small a miscalibration can be corrected for,
  // but also inherently assumes that a well-calibrated robot won't be used on
  // a surface that is inclined this much otherwise the head will calibrate
  // every time it's placed down. So we don't want it to be too small or big.
  // 10 degrees was a selected as a conservative value, but we should keep an eye out
  // for unnecessary head calibrations.
  if (std::fabsf(robot.GetPitchAngle().getDegrees()) > 10.f) {
    LOG_EVENT("BehaviorReactToReturnedToTreads.CalibratingHead", "%f", robot.GetPitchAngle().getDegrees());
    DelegateIfInControl(new CalibrateMotorAction(robot, true, false));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToReturnedToTreads::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
