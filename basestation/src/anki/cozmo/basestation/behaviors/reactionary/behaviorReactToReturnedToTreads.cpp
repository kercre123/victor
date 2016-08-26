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
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToReturnedToTreads");
  
  SubscribeToTriggerTags({
    EngineToGameTag::RobotOffTreadsStateChanged
  });
}

bool BehaviorReactToReturnedToTreads::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event,
                                                  const Robot& robot)
{
  if( event.GetTag() != EngineToGameTag::RobotOffTreadsStateChanged ) {
    PRINT_NAMED_ERROR("BehaviorReactToReturnedToTreads.ShouldRunForEvent.InvalidTag",
                      "Received trigger event with unhandled tag %hhu",
                      event.GetTag());
    return false;
  }
  
  return event.Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads;
} // ShouldRunForEvent()
  
  
bool BehaviorReactToReturnedToTreads::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToReturnedToTreads::InitInternalReactionary(Robot& robot)
{
  // No action to perform - simply switching to the behavior cancels animations
  return Result::RESULT_OK;
}


void BehaviorReactToReturnedToTreads::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
