/**
* File: behaviorFeedingEat.cpp
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to interact with an "energy" filled cube
* and drain the energy out of it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"



namespace Anki {
namespace Cozmo {
  
namespace{
const float kArbitraryWaitTime_s =  0.5;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::BehaviorFeedingEat(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{  
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData ) const
{
  if(ANKI_VERIFY(preReqData.GetTargets().size() == 1,
                 "BehaviorFeedingEat.PassedInInvalidNumberOfTargets",
                 "Passed in %zu targets",
                 preReqData.GetTargets().size())){
    _targetID = *preReqData.GetTargets().begin();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingEat::InitInternal(Robot& robot)
{
  TransitionToReactingToFood(robot);
  return Result::RESULT_OK;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::StopInternal(Robot& robot)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToFood(Robot& robot)
{
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::WorkoutPreLift_highEnergy),
              &BehaviorFeedingEat::TransitionToDrivingToFood);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new MoveLiftToHeightAction(robot, LIFT_HEIGHT_HIGHDOCK, 0));
  action->AddAction(new DriveToAlignWithObjectAction(robot, _targetID, 0, false, 0, AlignmentType::BODY));
  StartActing(action, &BehaviorFeedingEat::TransitionToEating);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToEating(Robot& robot)
{
  const char * feedingAnimName = "anim_bored_getout_01";
  // Extract the length of time that the animation will be playing for so that
  // it can be passed through to listeners
  Animation* anim = robot.GetContext()->GetRobotManager()->GetCannedAnimations().GetAnimation(feedingAnimName);
  for(auto & listener: _feedingListeners){
    if(ANKI_VERIFY(listener != nullptr,
                   "BehaviorFeedingEat.TransitionToEating.ListenerIsNull",
                   "")) {
      listener->StartedEating(robot, (anim->GetLastKeyFrameEndTime_ms()/1000));
    }
  }
  
  IActionRunner* action;
  action = new  PlayAnimationAction(robot, feedingAnimName);
  StartActing(action, &BehaviorFeedingEat::TransitionToWaiting);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToWaiting(Robot& robot)
{
  PRINT_NAMED_WARNING("BehaviorFeedingEat.TransitionToWaiting",
                      "This probably shouldn't happen - eating anim and drain time are out of sync");
  StartActing(new WaitAction(robot, kArbitraryWaitTime_s),
              &BehaviorFeedingEat::TransitionToWaiting);
}

} // namespace Cozmo
} // namespace Anki

