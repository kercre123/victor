/**
* File: behaviorFeedingHungerLoop.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to anticipate energy being loaded
* into a cube as the user prepares it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingHungerLoop.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static constexpr float kHungerTimeCycle_s = 5.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingHungerLoop::BehaviorFeedingHungerLoop(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingHungerLoop::IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingHungerLoop::InitInternal(Robot& robot)
{
  if(HasUsableFace(robot)){
    StartActing(new TurnTowardsLastFacePoseAction(robot),
                &BehaviorFeedingHungerLoop::TransitionAskForFood);
  }else{
    TransitionAskForFood(robot);
  }
  
  return Result::RESULT_OK;
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingHungerLoop::TransitionAskForFood(Robot& robot)
{
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::BuildPyramidThirdBlockUpright),
              &BehaviorFeedingHungerLoop::TransitionWaitForFood);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingHungerLoop::TransitionWaitForFood(Robot& robot)
{
  CompoundActionSequential* compound = new CompoundActionSequential(robot);
  compound->AddAction(new WaitAction(robot, kHungerTimeCycle_s));
  compound->AddAction(new SearchForNearbyObjectAction(robot));
  
  StartActing(compound, &BehaviorFeedingHungerLoop::TransitionWaitForFood);
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingHungerLoop::HasUsableFace(Robot& robot)
{
  Pose3d facePose;
  if(robot.GetFaceWorld().GetLastObservedFace(facePose)){
    const bool lastFaceInCurrentOrigin = (&facePose.FindOrigin() == robot.GetWorldOrigin());
    return lastFaceInCurrentOrigin;
  }
  
  return false;
}
  

} // namespace Cozmo
} // namespace Anki





