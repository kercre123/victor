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

#include "anki/cozmo/basestation/behaviors/behaviorReactToRobotOnSide.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
  
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnSide");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotOnSide
  });
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
  if( robot.IsOnSide()) {
    AnimationTrigger anim;
    if(_onRightSide){
      anim = AnimationTrigger::ReactToOnRightSide;
    }else{
      anim = AnimationTrigger::ReactToOnLeftSide;
    }
    
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::AskToBeRighted);
  }
}

void BehaviorReactToRobotOnSide::AskToBeRighted(Robot& robot)
{
  if( robot.IsOnSide()) {
    AnimationTrigger anim;
    if(_onRightSide){
      anim = AnimationTrigger::AskToBeRightedRight;
    }else{
      anim = AnimationTrigger::AskToBeRightedLeft;
    }
    
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}

  
void BehaviorReactToRobotOnSide::HoldingLoop(Robot& robot)
{
  if( robot.IsOnSide() ) {
    StartActing(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}

bool BehaviorReactToRobotOnSide::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if( event.GetTag() != MessageEngineToGameTag::RobotOnSide ) {
    PRINT_NAMED_ERROR("BehaviorReactToRobotOnSide.ShouldRunForEvent.BadEventType",
                      "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
    return false;
  }
  
  _onRightSide = event.Get_RobotOnSide().onRightSide;
  return true;
}

void BehaviorReactToRobotOnSide::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
