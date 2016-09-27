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
#include "anki/common/basestation/utils/timer.h"
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

namespace {
static const char* kTimeTilSleepAnimationKey = "timeTilSleepAnimation_s";
static const char* kTimeTilDisconnectionKey = "timeTilDisconnection_s";
const float kDisableReactionForInitialTime_sec = 20.f;
}

BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
  , _dontRunUntilTime_sec(-1.f)
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
  //ASSERT_NAMED(robot.IsOnChargerPlatform() || !robot.IsOnCharger(),
  //             "BehaviorDriveOffCharger.IsRunnableInternal.InconsistentChargerFlags");
  
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_dontRunUntilTime_sec < 0){
    _dontRunUntilTime_sec =  currentTime_sec + kDisableReactionForInitialTime_sec;
  }
  
  return _isOnCharger;
}

Result BehaviorReactToOnCharger::InitInternalReactionary(Robot& robot)
{
  // This is a hack - in order to have Cozmo drive off the charger if he accidentally roles back
  // onto the contacts because he saw an edge this reaction needs to start to cancel the cliff
  // detection reaction, and then return complete on the first update so that the DriveOffChargerBehavior can run
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_dontRunUntilTime_sec > currentTime_sec){
    _isOnCharger = false;
    return Result::RESULT_OK;
  }
  
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
