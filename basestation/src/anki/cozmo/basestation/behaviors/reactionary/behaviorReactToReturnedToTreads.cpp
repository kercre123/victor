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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToReturnedToTreads.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
BehaviorReactToReturnedToTreads::BehaviorReactToReturnedToTreads(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{

}

  
bool BehaviorReactToReturnedToTreads::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToReturnedToTreads::InitInternal(Robot& robot)
{
  // Wait for a bit to allow pitch to correct
  StartActing(new WaitAction(robot, 0.5f),
              &BehaviorReactToReturnedToTreads::CheckForHighPitch);
  
  return Result::RESULT_OK;
}
  
void BehaviorReactToReturnedToTreads::CheckForHighPitch(Robot& robot)
{
  // Check for pitch exceeding a certain threshold.
  // This threshold defines how small a miscalibration can be corrected for,
  // but also inherently assumes that a well-calibrated robot won't be used on
  // a surface that is inclined this much otherwise the head will calibrate
  // every time it's placed down. So we don't want it to be too small or big.
  // 10 degrees was a selected as a conservative value, but we should keep an eye out
  // for unnecessary head calibrations.
  if (std::fabsf(robot.GetPitchAngle().getDegrees()) > 10.f) {
    LOG_EVENT("BehaviorReactToReturnedToTreads.CalibratingHead", "%f", robot.GetPitchAngle().getDegrees());
    StartActing(new CalibrateMotorAction(robot, true, false));
  }
}

void BehaviorReactToReturnedToTreads::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
