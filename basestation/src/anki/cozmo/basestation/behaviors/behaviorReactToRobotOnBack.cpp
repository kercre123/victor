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

#include "anki/cozmo/basestation/behaviors/behaviorReactToRobotOnBack.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const char* const kFlipDownAnimGroupName = "FlipDownFromBack";
static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
  
BehaviorReactToRobotOnBack::BehaviorReactToRobotOnBack(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnBack");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotOnBack
  });
}

bool BehaviorReactToRobotOnBack::IsRunnableInternal(const Robot& robot) const
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
  if( robot.IsOnBack() ) {
    StartActing(new PlayAnimationGroupAction(robot, kFlipDownAnimGroupName),
                &BehaviorReactToRobotOnBack::DelayThenFlipDown);
  }
  else {
    SendFinishedFlipDownMessage(robot);
  }
}

void BehaviorReactToRobotOnBack::DelayThenFlipDown(Robot& robot)
{
  if( robot.IsOnBack() ) {
    StartActing(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
  else {
    SendFinishedFlipDownMessage(robot);
  }
}
    
void BehaviorReactToRobotOnBack::SendFinishedFlipDownMessage(Robot& robot){
  // Send message that we're done flipping
  robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotOnBackFinished()));
}

bool BehaviorReactToRobotOnBack::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if( event.GetTag() != MessageEngineToGameTag::RobotOnBack ) {
    PRINT_NAMED_ERROR("BehaviorReactToRobotOnBack.ShouldRunForEvent.BadEventType",
                      "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
    return false;
  }

  return event.Get_RobotOnBack().onBack;
}

void BehaviorReactToRobotOnBack::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
