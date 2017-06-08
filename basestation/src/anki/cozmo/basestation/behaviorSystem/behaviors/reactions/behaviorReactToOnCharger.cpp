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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToOnCharger.h"
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

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersOnCharger = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersOnCharger),
              "Reaction triggers duplicate or non-sequential");

}

BehaviorReactToOnCharger::BehaviorReactToOnCharger(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _onChargerCanceled(false)
{
  SubscribeToTags({
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
  
bool BehaviorReactToOnCharger::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToOnCharger::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersOnCharger);

  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::GoingToSleep>();
  robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PlacedOnCharger));
  robot.GetExternalInterface()->BroadcastToEngine<StartIdleTimeout>(_timeTilSleepAnimation_s, _timeTilDisconnect_s);
  return Result::RESULT_OK;
}
  
void BehaviorReactToOnCharger::StopInternal(Robot& robot)
{
  robot.GetAnimationStreamer().PopIdleAnimation();
}
  
IBehavior::Status BehaviorReactToOnCharger::UpdateInternal(Robot& robot)
{
  if( _onChargerCanceled )
  {
    _onChargerCanceled = false;
    return Status::Complete;
  }
  
  return Status::Running;
}
  
  
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
