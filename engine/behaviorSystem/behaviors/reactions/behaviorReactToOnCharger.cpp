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
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToOnCharger.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotIdleTimeoutComponent.h"
#include "engine/robotManager.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

namespace {
static const char* kTimeTilSleepAnimationKey = "timeTilSleepAnimation_s";
static const char* kTimeTilDisconnectionKey = "timeTilDisconnection_s";
static const char* kTriggeredFromVoiceCommandKey = "triggeredFromVoiceCommand";

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersOnCharger = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           true},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersOnCharger),
              "Reaction triggers duplicate or non-sequential");

} // namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
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
bool BehaviorReactToOnCharger::IsRunnableInternal(const Robot& robot) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToOnCharger::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersOnCharger);

  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::GoingToSleep>(_triggerableFromVoiceCommand);
  
  if(NeedId::Count == robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
    SmartPushIdleAnimation(robot, AnimationTrigger::Count);
  }
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PlacedOnCharger));
  robot.GetContext()->GetRobotManager()->GetMsgHandler()->SetRobotDisconnectReason(RobotDisconnectReason::SleepPlacedOnCharger);
  robot.GetExternalInterface()->BroadcastToEngine<StartIdleTimeout>(_timeTilSleepAnimation_s, _timeTilDisconnect_s);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToOnCharger::StopInternal(Robot& robot)
{
  _onChargerCanceled = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( _onChargerCanceled )
  {
    _onChargerCanceled = false;
    return Status::Complete;
  }
  
  return Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToOnCharger::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
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
