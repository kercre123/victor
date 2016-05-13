/**
 * File: behaviorReactToOnCharger.cpp
 *
 * Author: Molly
 * Created: 5/12/16
 *
 * Description: Behavior for going night night on charger
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorReactToOnCharger.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kSleepStartAG = "ag_reactToCharger";
static const char* kSleepLoopAG = "ag_SleepLoop";

BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToOnCharger");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::ChargerEvent
  });
}

bool BehaviorReactToOnCharger::IsRunnableInternal(const Robot& robot) const
{
  return robot.IsOnCharger();
}

Result BehaviorReactToOnCharger::InitInternal(Robot& robot)
{
  StartActing(new PlayAnimationGroupAction(robot, kSleepStartAG),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  
  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( robot.IsOnCharger() )
  {
    return Status::Running;
  }
  
  return Status::Complete;
}
  
void BehaviorReactToOnCharger::StopInternal(Robot& robot)
{
}

bool BehaviorReactToOnCharger::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event) const
{
  if( event.GetTag() != MessageEngineToGameTag::ChargerEvent )
  {
    PRINT_NAMED_ERROR("BehaviorReactToOnCharger.ShouldRunForEvent.BadEventType",
                      "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
    return false;
  }
  
  return event.Get_ChargerEvent().onCharger;
}
  
void BehaviorReactToOnCharger::TransitionToSleepLoop(Robot& robot)
{
  if( robot.IsOnCharger() )
  {
    StartActing(new PlayAnimationGroupAction(robot, kSleepLoopAG),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  }
}

} // namespace Cozmo
} // namespace Anki
