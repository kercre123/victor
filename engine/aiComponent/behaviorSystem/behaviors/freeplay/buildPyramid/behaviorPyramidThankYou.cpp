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

#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "anki/common/basestation/utils/timer.h"



namespace Anki {
namespace Cozmo {

namespace {
const float kTimeSinceFaceSeenForTurn = 20;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPyramidThankYou::BehaviorPyramidThankYou(const Json::Value& config)
: IBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPyramidThankYou::~BehaviorPyramidThankYou()
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPyramidThankYou::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // Check to see if there's a person we can turn to look at
  if(behaviorExternalInterface.GetFaceWorld().HasAnyFaces(kTimeSinceFaceSeenForTurn)){
    return true;
  }
  
  // Check to see if there's a block with a location we can turn to to say thanks
  if(_targetID > -1){
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(obj != nullptr){
      return true;
    }
  }
  
  _targetID = -1;
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPyramidThankYou::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  CompoundActionSequential* turnVerifyThank = new CompoundActionSequential(robot);
  Pose3d facePose;
  behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(facePose);
  const bool lastFaceInCurrentOrigin = robot.IsPoseInWorldOrigin(facePose);
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
Result BehaviorPyramidThankYou::ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // don't resume - if the animation was interrupted the moment is gone
  return Result::RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPyramidThankYou::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetID = -1;
}

}
}
