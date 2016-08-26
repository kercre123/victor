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
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnBack");


}

  
bool BehaviorReactToRobotOnBack::ShouldComputationallySwitch(const Robot& robot)
{
  return robot.GetOffTreadsState() == OffTreadsState::OnBack;
}


bool BehaviorReactToRobotOnBack::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToRobotOnBack::InitInternalReactionary(Robot& robot)
{
  FlipDownIfNeeded(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnBack::FlipDownIfNeeded(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnBack ) {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FlipDownFromBack),
                &BehaviorReactToRobotOnBack::DelayThenFlipDown);
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

void BehaviorReactToRobotOnBack::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
