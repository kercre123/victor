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

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnSide.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 15.f;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnSide::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToRobotOnSide::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // clear bored animation timer
  _timeToPerformBoredAnim_s = -1.0f;
  
  // play meter damage
  NeedActionCompleted(NeedsActionId::PlacedOnSide);
  
  ReactToBeingOnSide(behaviorExternalInterface);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::ReactToBeingOnSide(BehaviorExternalInterface& behaviorExternalInterface)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::ReactToOnLeftSide;
  }
  
  if(behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::ReactToOnRightSide;
  }
  
  if(anim != AnimationTrigger::Count){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::AskToBeRighted);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::AskToBeRighted(BehaviorExternalInterface& behaviorExternalInterface)
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::AskToBeRightedLeft;
  }
  
  if(behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::AskToBeRightedRight;
  }
  
  if(anim != AnimationTrigger::Count){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::HoldingLoop(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnRightSide
     || behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnLeftSide) {

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    if( _timeToPerformBoredAnim_s < 0.0f ) {
      // set timer for when to perform the bored animations
      _timeToPerformBoredAnim_s = currTime_s + kWaitTimeBeforeRepeatAnim_s;
    }
    
    if( currTime_s >= _timeToPerformBoredAnim_s ) {
      // reset timer
      _timeToPerformBoredAnim_s = -1.0f;

      // play meter damage
      NeedActionCompleted(NeedsActionId::BoredOnSide);
      
      // play bored animation sequence, then return to holding
      
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      // note: NothingToDoBored anims can move the robot, so Intro/Outro may not work here well, should
      // we be playing a specific loop here?
      DelegateIfInControl(new CompoundActionSequential(robot, {
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredIntro),
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredEvent),
                    new TriggerAnimationAction(robot, AnimationTrigger::NothingToDoBoredOutro) }),
                  &BehaviorReactToRobotOnSide::HoldingLoop);
    }
    else {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      // otherwise, we just loop this animation
      DelegateIfInControl(new TriggerAnimationAction(robot, AnimationTrigger::WaitOnSideLoop),
                  &BehaviorReactToRobotOnSide::HoldingLoop);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
