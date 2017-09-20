/**
 * File: BehaviorDevPettingTestSimple.cpp
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/behaviorDevPettingTestSimple.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/components/touchSensorComponent.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
  const float kPettingReactRecoveryTime = 1.0f; // rate limit reactions to touch
}

  
BehaviorDevPettingTestSimple::BehaviorDevPettingTestSimple(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _lastTouchTime_s(-1.0f)
, _lastReactTime_s(-1.0f)
{
  SubscribeToTags({
    EngineToGameTag::RobotTouched,
  });
}
  
void BehaviorDevPettingTestSimple::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotTouched:
    {
      HandleRobotTouched(robot, event);
      break;
    }
      
    default:
    {
      PRINT_NAMED_WARNING("BehaviorDevPettingTestSimple.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
    }
  }
}
  
void BehaviorDevPettingTestSimple::HandleRobotTouched(const Robot& robot, const EngineToGameEvent& msg)
{
  _lastTouchTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  

bool BehaviorDevPettingTestSimple::IsRunnableInternal(const Robot& robot) const
{
  return (ANKI_DEV_CHEATS != 0);
}
  

Result BehaviorDevPettingTestSimple::InitInternal(Robot& robot)
{
  // Disable all reactions
  SmartDisableReactionsWithLock(GetIDStr(), ReactionTriggerHelpers::kAffectAllArray);
  
  return RESULT_OK;
}
  
BehaviorStatus BehaviorDevPettingTestSimple::UpdateInternal(Robot& robot)
{
  const AnimationTrigger devReactAnimation = AnimationTrigger::RequestGameMemoryMatchAccept0;
  
  // throttle touch reactions (animations) over time
  // note: if you pet the robot during the "recovery" time, it'll
  //       react as soon as that duration times out.
  auto now =  BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!IsActing() &&
     _lastTouchTime_s > 0.0 &&
     (now-_lastReactTime_s)>kPettingReactRecoveryTime ) {
    
    _lastReactTime_s = now;
    
    TriggerAnimationAction* action = new TriggerAnimationAction(robot, devReactAnimation);
    StartActing(action,
                [this](ActionResult result) {
                  _lastTouchTime_s = -1.0f;
                });
  }
  
  return BehaviorStatus::Running;
}


void BehaviorDevPettingTestSimple::StopInternal(Robot& robot)
{
}

} // Cozmo
} // Anki
