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

// this is a stand-in for the real pickup animation
static const char* kPickupReactAnimName = "anim_keepAlive_blink_01";

// TODO:(bn) put this somewhere central? Maybe just make a robot.IsOnBack() function? Where should I put the
// config?
static const float kPitchAngleOnBack_rads = DEG_TO_RAD(74.5f);
static const float kPitchAngleOnBack_sim_rads = DEG_TO_RAD(96.4f);

CONSOLE_VAR(float, kPitchAngleOnBackTolerance_deg, "BehaviorReactToPickup", 5.0f);
CONSOLE_VAR(float, kTimeToConsiderOnBack_s, "BehaviorReactToPickup", 0.3f);

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
  if( _isInAir ) {
    const float backAngle = robot.IsPhysical() ? kPitchAngleOnBack_rads : kPitchAngleOnBack_sim_rads;
      
    const bool onBack = std::abs( robot.GetPitchAngle() - backAngle ) <= DEG_TO_RAD( kPitchAngleOnBackTolerance_deg );

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    if( onBack ) {
      if( _timeToConsiderOnBack < 0.0f ) {
        _timeToConsiderOnBack = currTime_s + kTimeToConsiderOnBack_s;
      }
      else if( currTime_s >= _timeToConsiderOnBack ) {
        // we've been "on our back" for long enough now, so consider the robot to be put down
        _isInAir = false;
        _timeToConsiderOnBack = -1.0f;
      }
    }
    else {
      _timeToConsiderOnBack = -1.0f;
    }
  }
  
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
      _timeToConsiderOnBack = -1.0f;
      break;
    }
    case MessageEngineToGameTag::RobotPutDown:
    {
      _isInAir = false;
      _timeToConsiderOnBack = -1.0f;
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
