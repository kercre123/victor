/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnBack.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
  
BehaviorReactToRobotOnBack::BehaviorReactToRobotOnBack(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnBack");
}


bool BehaviorReactToRobotOnBack::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToRobotOnBack::InitInternal(Robot& robot)
{
  FlipDownIfNeeded(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnBack::FlipDownIfNeeded(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnBack ) {
    
    // Check if cliff detected
    // If not, then calibrate head because we're not likely to be on back if no cliff detected.
    if (robot.GetCliffDataRaw() < CLIFF_SENSOR_DROP_LEVEL) {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FlipDownFromBack),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    } else {
      LOG_EVENT("BehaviorReactToRobotOnBack.FlipDownIfNeeded.CalibratingHead", "%d", robot.GetCliffDataRaw());
      StartActing(new CalibrateMotorAction(robot, true, false),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    }
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}

void BehaviorReactToRobotOnBack::DelayThenFlipDown(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnBack ) {
    StartActing(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}

void BehaviorReactToRobotOnBack::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
