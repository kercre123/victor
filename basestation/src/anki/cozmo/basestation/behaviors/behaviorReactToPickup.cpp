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

#include "anki/cozmo/basestation/behaviors/behaviorReactToPickup.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

// this is a stand-in for the real pickup animation
static const char* kPickupReactAnimName = "anim_keepAlive_blink_01";

BehaviorReactToPickup::BehaviorReactToPickup(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToPickup");
 
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotPickedUp
  });
  
  // These are additional tags that this behavior should handle
  SubscribeToTags({
    EngineToGameTag::RobotPutDown
  });

}

bool BehaviorReactToPickup::IsRunnable(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToPickup::InitInternal(Robot& robot)
{
  StartActing(new PlayAnimationAction(robot, kPickupReactAnimName));
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPickup::UpdateInternal(Robot& robot)
{
  if( IsActing() || _isInAir ) {
    return Status::Running;
  }
  
  return Status::Complete;
}
  
void BehaviorReactToPickup::StopInternal(Robot& robot)
{
}

void BehaviorReactToPickup::AlwaysHandle(const EngineToGameEvent& event,
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
    default:
    {
      break;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
