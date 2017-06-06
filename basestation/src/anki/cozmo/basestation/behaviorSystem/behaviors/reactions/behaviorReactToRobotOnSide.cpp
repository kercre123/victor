/**
 * File: behaviorReactToRobotOnSide.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Cozmo reacts to being placed on his side
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnSide.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 15.f;
  
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}


bool BehaviorReactToRobotOnSide::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToRobotOnSide::InitInternal(Robot& robot)
{
  // clear bored animation timer
  _timeToPerformBoredAnim_s = -1.0f;
  
  ReactToBeingOnSide(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnSide::ReactToBeingOnSide(Robot& robot)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( robot.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::ReactToOnLeftSide;
  }
  
  if(robot.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::ReactToOnRightSide;
  }
  
  if(anim != AnimationTrigger::Count){
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::AskToBeRighted);
  }
}

void BehaviorReactToRobotOnSide::AskToBeRighted(Robot& robot)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( robot.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::AskToBeRightedLeft;
  }
  
  if(robot.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::AskToBeRightedRight;
  }
  
  if(anim != AnimationTrigger::Count){
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}

  
void BehaviorReactToRobotOnSide::HoldingLoop(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnRightSide
     || robot.GetOffTreadsState() == OffTreadsState::OnLeftSide) {

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    if( _timeToPerformBoredAnim_s < 0.0f ) {
      // set timer for when to perform the bored animations
      _timeToPerformBoredAnim_s = currTime_s + kWaitTimeBeforeRepeatAnim_s;
    }
    
    if( currTime_s >= _timeToPerformBoredAnim_s ) {
      // reset timer
      _timeToPerformBoredAnim_s = -1.0f;

      // play bored animation sequence, then return to holding
      
      // note: NothingToDoBored anims can move the robot, so Intro/Outro may not work here well, should
      // we be playing a specific loop here?
      StartActing(new CompoundActionSequential(robot, {
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredIntro),
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredEvent),
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredOutro) }),
                  &BehaviorReactToRobotOnSide::HoldingLoop);
    }
    else {
      // otherwise, we just loop this animation
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::WaitOnSideLoop),
                  &BehaviorReactToRobotOnSide::HoldingLoop);
    }
  }
}

void BehaviorReactToRobotOnSide::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
