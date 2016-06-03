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
    EngineToGameTag::ChargerEvent,
  });
  
  SubscribeToTags({
    GameToEngineTag::StartDemoWithEdge
  });
}

bool BehaviorReactToOnCharger::IsRunnableInternal(const Robot& robot) const
{
  return robot.IsOnCharger();
}

Result BehaviorReactToOnCharger::InitInternal(Robot& robot)
{
  _shouldStopBehavior = false;
  StartActing(new PlayAnimationGroupAction(robot, kSleepStartAG),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  
  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( robot.IsOnCharger() && !_shouldStopBehavior )
  {
    return Status::Running;
  }
  
  return Status::Complete;
}

bool BehaviorReactToOnCharger::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  // Don't restart the behavior unless we got off the platform
  // This will prevents little bumps causing us to restart
  if(event.GetTag() == MessageEngineToGameTag::ChargerEvent)
  {
    return event.Get_ChargerEvent().onCharger;
  }
  
  return false;
}

void BehaviorReactToOnCharger::TransitionToSleepLoop(Robot& robot)
{
  if( robot.IsOnCharger() )
  {
    StartActing(new PlayAnimationGroupAction(robot, kSleepLoopAG),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  }
}
  
void BehaviorReactToOnCharger::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::StartDemoWithEdge:
    {
      _shouldStopBehavior = true;
      break;
    }
    default:
    {
      break;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
