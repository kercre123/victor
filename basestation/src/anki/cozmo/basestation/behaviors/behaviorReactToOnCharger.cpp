/**
 * File: behaviorReactToOnCharger.cpp
 *
 * Author: Molly
 * Created: 5/12/16
 *
 * Description: Behavior for going night night on charger
 *
 * Copyright: Anki, Inc. 2016
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

BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToOnCharger");
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::ChargerEvent,
  });
  
  SubscribeToTags({
    GameToEngineTag::ContinueFreeplayFromIdle,
  });
  SubscribeToTags({
    EngineToGameTag::RobotPickedUp,
  });
  
}
// It's pretty easy for him to get nudged off and back on the charger, so make sure he has left the platform at least once
bool BehaviorReactToOnCharger::IsRunnableInternalReactionary(const Robot& robot) const
{
  return _isOnCharger;
}

Result BehaviorReactToOnCharger::InitInternalReactionary(Robot& robot)
{
  _shouldStopBehavior = false;
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::PlacedOnCharger),&BehaviorReactToOnCharger::TransitionToIdleLoop);
  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( _isOnCharger && !_shouldStopBehavior )
  {
    return Status::Running;
  }
  
  return Status::Complete;
}

bool BehaviorReactToOnCharger::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if(event.GetTag() == MessageEngineToGameTag::ChargerEvent)
  {
    _isOnCharger = event.Get_ChargerEvent().onCharger;
    return _isOnCharger;
  }
  
  return false;
}

void BehaviorReactToOnCharger::TransitionToIdleLoop(Robot& robot)
{
  if( _isOnCharger )
  {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::IdleOnCharger),&BehaviorReactToOnCharger::TransitionToIdleLoop);
  }
}
  
void BehaviorReactToOnCharger::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::ContinueFreeplayFromIdle:
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

void BehaviorReactToOnCharger::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case EngineToGameTag::RobotPickedUp:
      _shouldStopBehavior = true;
      break;
      
    default:
      break;
  }
}

  

} // namespace Cozmo
} // namespace Anki
