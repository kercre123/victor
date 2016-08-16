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
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToOnCharger.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotIdleTimeoutComponent.h"
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
  
}
  
bool BehaviorReactToOnCharger::IsRunnableInternalReactionary(const Robot& robot) const
{
  return _isOnCharger;
}

Result BehaviorReactToOnCharger::InitInternalReactionary(Robot& robot)
{
  robot.GetBehaviorManager().SetReactionaryBehaviorsEnabled(false);
  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::GoingToSleep>();
  
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::PlacedOnCharger),&BehaviorReactToOnCharger::TransitionToIdleLoop);
  return Result::RESULT_OK;
}
  
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( _isOnCharger )
  {
    return Status::Running;
  }
  
  return Status::Complete;
}

bool BehaviorReactToOnCharger::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if(event.GetTag() == MessageEngineToGameTag::ChargerEvent && event.Get_ChargerEvent().onCharger)
  {
    _isOnCharger = true;
    return _isOnCharger;
  }
  
  return false;
}

void BehaviorReactToOnCharger::TransitionToIdleLoop(Robot& robot)
{
  auto doDisconnect = [] (Robot& robot) { robot.GetRobotMessageHandler()->Disconnect(); };
  
  StartActing(RobotIdleTimeoutComponent::CreateGoToSleepAnimSequence(robot), doDisconnect);
}

} // namespace Cozmo
} // namespace Anki
