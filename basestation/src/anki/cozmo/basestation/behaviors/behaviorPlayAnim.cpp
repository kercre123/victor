/**
 * File: behaviorPlayAnim
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kAnimTriggerKey = "animTrigger";
static const char* kLoopsKey = "num_loops";

BehaviorPlayAnim::BehaviorPlayAnim(Robot& robot, const Json::Value& config, bool keyRequired )
  : IBehavior(robot, config)
{
  SetDefaultName("PlayAnim");
  
  if (!config.isNull())
  {
    JsonTools::GetValueOptional(config,kAnimTriggerKey,_animTrigger);
  }
  
  if(keyRequired){
    ASSERT_NAMED(config.isMember(kAnimTriggerKey), "Invalid animation trigger key");
  }
  _numLoops = config.get(kLoopsKey, 1).asInt();
}
    
BehaviorPlayAnim::~BehaviorPlayAnim()
{  
}
  
bool BehaviorPlayAnim::IsRunnableInternal(const Robot& robot) const
{
  const bool retVal = true;
  return retVal;
}

Result BehaviorPlayAnim::InitInternal(Robot& robot)
{
  const bool interruptRunning = true;
  u8 tracksToLock = (u8) AnimTrackFlag::NO_TRACKS;
  
  // Ensure animation doesn't throw cube down, but still can play get down animations
  if(robot.IsCarryingObject()
     && robot.GetOffTreadsState() == OffTreadsState::OnTreads){
    tracksToLock = (u8) AnimTrackFlag::LIFT_TRACK;
  }
  
  StartActing(new TriggerAnimationAction(robot, _animTrigger, _numLoops, interruptRunning, tracksToLock));
  return Result::RESULT_OK;
}  

} // namespace Cozmo
} // namespace Anki
