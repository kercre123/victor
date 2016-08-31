/**
 * File: behaviorReactToRobotOnSide.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Cozmo reacts to being placed on his side
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnSide.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 15.f;
  
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnSide");
}
  
  
bool BehaviorReactToRobotOnSide::ShouldComputationallySwitch(const Robot& robot)
{
  return robot.GetOffTreadsState() == OffTreadsState::OnLeftSide
      || robot.GetOffTreadsState() == OffTreadsState::OnRightSide;
}

bool BehaviorReactToRobotOnSide::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToRobotOnSide::InitInternalReactionary(Robot& robot)
{
  ReactToBeingOnSide(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnSide::ReactToBeingOnSide(Robot& robot)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( robot.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::ReactToOnLeftSide;
  }
  
  if(robot.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::ReactToOnRightSide;
  }
  
  if(anim != AnimationTrigger::Count){
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::AskToBeRighted);
  }
}

void BehaviorReactToRobotOnSide::AskToBeRighted(Robot& robot)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( robot.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::AskToBeRightedLeft;
  }
  
  if(robot.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::AskToBeRightedRight;
  }
  
  if(anim != AnimationTrigger::Count){
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}

  
void BehaviorReactToRobotOnSide::HoldingLoop(Robot& robot)
{
  
  if( robot.GetOffTreadsState() == OffTreadsState::OnRightSide
     || robot.GetOffTreadsState() == OffTreadsState::OnLeftSide) {
    StartActing(new CompoundActionSequential(robot, {
      new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
      // note: NothingToDoBored anims can move the robot, so Intro/Outro may not work here well, should
      // we be playing a specific loop here?
      new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredEvent)
    }),  &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}

void BehaviorReactToRobotOnSide::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
