/**
 * File: behaviorPyramidThankYou.cpp
 *
 * Author: Kevin M. Karol
 * Created: 01/24/16
 *
 * Description: Behavior to thank users for putting a block upright
 * when it was on its side for build pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"



namespace Anki {
namespace Cozmo {

namespace {
const float kTimeSinceFaceSeenForTurn = 20;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPyramidThankYou::BehaviorPyramidThankYou(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPyramidThankYou::~BehaviorPyramidThankYou()
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPyramidThankYou::IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData) const
{
  // Check to see if there's a person we can turn to look at
  if(_robot.GetFaceWorld().HasAnyFaces(kTimeSinceFaceSeenForTurn)){
    return true;
  }
  
  // Check to see if there's a block with a location we can turn to to say thanks
  if(preReqData.GetTargets().size() >= 1){
    _targetID = *preReqData.GetTargets().begin();
    const ObservableObject* obj = _robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(obj != nullptr){
      return true;
    }
  }

  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPyramidThankYou::InitInternal(Robot& robot)
{

  CompoundActionSequential* turnVerifyThank = new CompoundActionSequential(robot);
  Pose3d facePose;
  robot.GetFaceWorld().GetLastObservedFace(facePose);
  const bool lastFaceInCurrentOrigin = &facePose.FindOrigin() == robot.GetWorldOrigin();
  if(lastFaceInCurrentOrigin){
   // Turn to the user and say thank you
    TurnTowardsFaceAction* turnTowardsAction = new TurnTowardsLastFacePoseAction(robot);
    turnTowardsAction->SetRequireFaceConfirmation(true);
    turnVerifyThank->AddAction(turnTowardsAction);
    turnVerifyThank->AddAction(new TriggerAnimationAction(robot,
                                      AnimationTrigger::BuildPyramidThankUser));
  }else{
    // Turn to the block that was just righted, and say thanks
    turnVerifyThank->AddAction(new TurnTowardsObjectAction(robot, _targetID,
                                                           Radians(M_PI_F), true));
    turnVerifyThank->AddAction(new TriggerAnimationAction(robot,
                                      AnimationTrigger::BuildPyramidThankUser));
  }
  
  StartActing(turnVerifyThank);
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPyramidThankYou::ResumeInternal(Robot& robot)
{
  // don't resume - if the animation was interrupted the moment is gone
  return Result::RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPyramidThankYou::StopInternal(Robot& robot)
{
  
}

}
}
