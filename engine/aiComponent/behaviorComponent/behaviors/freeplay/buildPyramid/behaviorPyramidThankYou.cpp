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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/faceWorld.h"
#include "anki/common/basestation/utils/timer.h"



namespace Anki {
namespace Cozmo {

namespace {
const float kTimeSinceFaceSeenForTurn = 20;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPyramidThankYou::BehaviorPyramidThankYou(const Json::Value& config)
: ICozmoBehavior(config)
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
void BehaviorPyramidThankYou::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  CompoundActionSequential* turnVerifyThank = new CompoundActionSequential();
  Pose3d facePose;
  behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(facePose);
  const bool lastFaceInCurrentOrigin = behaviorExternalInterface.GetRobotInfo().IsPoseInWorldOrigin(facePose);
  if(lastFaceInCurrentOrigin){
   // Turn to the user and say thank you
    TurnTowardsFaceAction* turnTowardsAction = new TurnTowardsLastFacePoseAction();
    turnTowardsAction->SetRequireFaceConfirmation(true);
    turnVerifyThank->AddAction(turnTowardsAction);
    turnVerifyThank->AddAction(new TriggerAnimationAction(AnimationTrigger::BuildPyramidThankUser));
  }else{
    // Turn to the block that was just righted, and say thanks
    turnVerifyThank->AddAction(new TurnTowardsObjectAction(_targetID,
                                                           Radians(M_PI_F), true));
    turnVerifyThank->AddAction(new TriggerAnimationAction(AnimationTrigger::BuildPyramidThankUser));
  }
  
  DelegateIfInControl(turnVerifyThank);
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPyramidThankYou::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetID = -1;
}

}
}
