/**
 * File: behaviorReactToPickup.cpp
 *
 * Author: Lee
 * Created: 08/26/15
 *
 * Description: Behavior for immediately responding being picked up.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPickup.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

CONSOLE_VAR(f32, kMinTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 3.0f);
CONSOLE_VAR(f32, kMaxTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 6.0f);
CONSOLE_VAR(f32, kRepeatAnimMultIncrease, "Behavior.ReactToPickup", 0.33f);

BehaviorReactToPickup::BehaviorReactToPickup(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToPickup");
 
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotPickedUp
  });
  
  // These are additional tags that this behavior should handle
  SubscribeToTags({{
    EngineToGameTag::RobotPutDown,
    EngineToGameTag::RobotOnBack
  }});

}

bool BehaviorReactToPickup::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  ASSERT_NAMED( event.GetTag() == EngineToGameTag::RobotPickedUp, "BehaviorReactToPickup.TriggerForWrongTag" );

  // always run, unless we are on the charger or playing alternate reactionary behavior
  // TODO:(bn) a reaction for being carried in the charger?
  return ! (robot.IsOnCharger() || robot.IsOnSide() || robot.IsOnBack() || robot.IsOnFace());
}

bool BehaviorReactToPickup::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToPickup::InitInternalReactionary(Robot& robot)
{
  _repeatAnimatingMultiplier = 1;
  StartAnim(robot);
  return Result::RESULT_OK;
}
  
void BehaviorReactToPickup::StartAnim(Robot& robot)
{
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToPickup));
  
  const double minTime = _repeatAnimatingMultiplier * kMinTimeBetweenPickupAnims_sec;
  const double maxTime = _repeatAnimatingMultiplier * kMaxTimeBetweenPickupAnims_sec;
  const double nextInterval = GetRNG().RandDblInRange(minTime, maxTime);
  _nextRepeatAnimationTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + nextInterval;
  _repeatAnimatingMultiplier += kRepeatAnimMultIncrease;
}
  
IBehavior::Status BehaviorReactToPickup::UpdateInternal(Robot& robot)
{
  const bool isActing = IsActing();
  if( !isActing && !_isInAir ) {
    return Status::Complete;
  }

  if( robot.IsOnCharger() && !isActing ) {
    PRINT_NAMED_INFO("BehaviorReactToPickup.OnCharger", "Stopping behavior because we are on the charger");
    return Status::Complete;
  }
  // If we aren't acting, it might be time to play another reaction
  if (!isActing)
  {
    const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (currentTime > _nextRepeatAnimationTime)
    {
      StartAnim(robot);
    }
  }
  
  return Status::Running;
}
  
void BehaviorReactToPickup::StopInternalReactionary(Robot& robot)
{
}

void BehaviorReactToPickup::AlwaysHandleInternal(const EngineToGameEvent& event,
                                         const Robot& robot)
{
  // We want to get these messages, even when not running
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotPickedUp:
    {
      _isInAir = true;
      break;
    }
    case MessageEngineToGameTag::RobotPutDown:
    {
      _isInAir = false;
      break;
    }
    case MessageEngineToGameTag::RobotOnBack:
    {
      if( event.GetData().Get_RobotOnBack().onBack ) {
        _isInAir = false;
      }
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
