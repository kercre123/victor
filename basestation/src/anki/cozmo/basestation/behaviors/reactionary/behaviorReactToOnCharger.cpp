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

#include "anki/common/basestation/jsonTools.h"
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
  
static const char* kTimeTilSleepAnimationKey = "timeTilSleepAnimation_s";
static const char* kTimeTilDisconnectionKey = "timeTilDisconnection_s";

BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToOnCharger");
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::ChargerEvent,
  });
  
  SubscribeToTags({
    GameToEngineTag::CancelIdleTimeout
  });
  
  if (!JsonTools::GetValueOptional(config, kTimeTilSleepAnimationKey, _timeTilSleepAnimation_s))
  {
    PRINT_NAMED_ERROR("BehaviorReactToOnCharger.Constructor", "Config missing value for key: %s", kTimeTilSleepAnimationKey);
  }
  
  if (!JsonTools::GetValueOptional(config, kTimeTilDisconnectionKey, _timeTilDisconnect_s))
  {
    PRINT_NAMED_ERROR("BehaviorReactToOnCharger.Constructor", "Config missing value for key: %s", kTimeTilDisconnectionKey);
  }
}
  
bool BehaviorReactToOnCharger::IsRunnableInternalReactionary(const Robot& robot) const
{
  // assumes it's not possible to be OnCharger without being OnChargerPlatform
  const ObservableObject* charger = robot.GetBlockWorld().GetObjectByID(robot.GetCharger(), ObjectFamily::Charger);
  ASSERT_NAMED((charger == nullptr) || robot.IsOnChargerPlatform() || !robot.IsOnCharger(),
               "BehaviorReactToOnCharger.IsRunnableInternal.InconsistentChargerFlags");
  
  return _isOnCharger;
}

Result BehaviorReactToOnCharger::InitInternalReactionary(Robot& robot)
{
  robot.GetBehaviorManager().SetReactionaryBehaviorsEnabled(false);
  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::GoingToSleep>();
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PlacedOnCharger));
  robot.GetExternalInterface()->BroadcastToEngine<StartIdleTimeout>(_timeTilSleepAnimation_s, _timeTilDisconnect_s);
  return Result::RESULT_OK;
}
  
void BehaviorReactToOnCharger::StopInternalReactionary(Robot& robot)
{
  robot.GetBehaviorManager().SetReactionaryBehaviorsEnabled(true);
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
  
void BehaviorReactToOnCharger::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case GameToEngineTag::CancelIdleTimeout:
    {
      _isOnCharger = false;
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
