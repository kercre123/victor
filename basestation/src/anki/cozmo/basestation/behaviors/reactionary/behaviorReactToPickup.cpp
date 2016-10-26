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

}
  
bool BehaviorReactToPickup::ShouldComputationallySwitch(const Robot& robot)
{
  return robot.GetOffTreadsState() == OffTreadsState::InAir;
}
  
bool BehaviorReactToPickup::IsRunnableInternalReactionary(const Robot& robot) const
{
  return robot.GetOffTreadsState() == OffTreadsState::InAir;
}
  

Result BehaviorReactToPickup::InitInternalReactionary(Robot& robot)
{
  _repeatAnimatingMultiplier = 1;
  
  // Delay introduced since cozmo can be marked as "In air" in robot.cpp while we
  // wait for the cliffDetect sensor to confirm he's on the ground
  const f32 bufferDelay_s = .5f;
  const f32 wait_s = CLIFF_EVENT_DELAY_MS/1000 + bufferDelay_s;
  StartActing(new WaitAction(robot, wait_s), &BehaviorReactToPickup::StartAnim);
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
  if( !isActing && robot.GetOffTreadsState() != OffTreadsState::InAir ) {
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
      if (robot.GetCliffDataRaw() < CLIFF_SENSOR_DROP_LEVEL) {
        StartAnim(robot);
      } else {
        PRINT_NAMED_EVENT("BehaviorReactToPickup.CalibratingHead",
                          "%d", robot.GetCliffDataRaw());
        StartActing(new CalibrateMotorAction(robot, true, false));
      }
    }
  }
  
  return Status::Running;
}
  
void BehaviorReactToPickup::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
