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

// DEMO HACK
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/demoBehaviorChooser.h"

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
    GameToEngineTag::WakeUp
  });
  SubscribeToTags({
    EngineToGameTag::DemoState
  });
  
}
// It's pretty easy for him to get nudged off and back on the charger, so make sure he has left the platform at least once
bool BehaviorReactToOnCharger::IsRunnableReactionaryInternal(const Robot& robot) const
{
  return _isOnCharger && _isReactionEnabled;
}

Result BehaviorReactToOnCharger::InitInternalReactionary(Robot& robot)
{
  _shouldStopBehavior = false;
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::OnChargerStartSleeping),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  
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

void BehaviorReactToOnCharger::TransitionToSleepLoop(Robot& robot)
{
  if( _isOnCharger )
  {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::OnChargerLoopSleeping),&BehaviorReactToOnCharger::TransitionToSleepLoop);
  }
}

void BehaviorReactToOnCharger::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    // DEMO HACK ( obviously )
    case EngineToGameTag::DemoState:
    {
      if( event.GetData().Get_DemoState().stateNum >= Util::EnumToUnderlying(DemoBehaviorChooser::State::FearEdge) )
      {
        _isReactionEnabled = true;
      }
      
      break;
    }
    default:
    {
      break;
    }
  }
}
  
void BehaviorReactToOnCharger::AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    // DEMO HACK ( obviously )
    // This is to make it more reliable in the demo that his wake up animation doesn't force him back
    // on the charger to go to sleep.
    case GameToEngineTag::WakeUp:
    {
      _isReactionEnabled = false;
      break;
    }
    default:
    {
      break;
    }
  }
}
  
void BehaviorReactToOnCharger::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::WakeUp:
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
