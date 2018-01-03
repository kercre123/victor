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
#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToOnCharger.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotIdleTimeoutComponent.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

namespace {
static const char* kTimeTilSleepAnimationKey = "timeTilSleepAnimation_s";
static const char* kTimeTilDisconnectionKey = "timeTilDisconnection_s";
static const char* kTriggeredFromVoiceCommandKey = "triggeredFromVoiceCommand";

} // namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToOnCharger::BehaviorReactToOnCharger(const Json::Value& config)
: ICozmoBehavior(config)
, _onChargerCanceled(false)
, _timeTilSleepAnimation_s(-1.0)
, _timeTilDisconnect_s(0.0)
, _triggerableFromVoiceCommand(false)
{
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
  
  JsonTools::GetValueOptional(config, kTriggeredFromVoiceCommandKey, _triggerableFromVoiceCommand);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToOnCharger::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToOnCharger::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  /**auto externalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(externalInterface != nullptr){
    externalInterface->BroadcastToGame<ExternalInterface::GoingToSleep>(_triggerableFromVoiceCommand);
    externalInterface->BroadcastToEngine<StartIdleTimeout>(_timeTilSleepAnimation_s, _timeTilDisconnect_s);
  }**/
  
  if(NeedId::Count == behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
    SmartPushIdleAnimation(behaviorExternalInterface, AnimationTrigger::Count);
  }
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PlacedOnCharger));
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToOnCharger::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _onChargerCanceled = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorReactToOnCharger::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _onChargerCanceled )
  {
    _onChargerCanceled = false;
    return Status::Complete;
  }
  
  return Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToOnCharger::HandleWhileActivated(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch (event.GetData().GetTag())
  {
    case GameToEngineTag::CancelIdleTimeout:
    {
      _onChargerCanceled = true;
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
