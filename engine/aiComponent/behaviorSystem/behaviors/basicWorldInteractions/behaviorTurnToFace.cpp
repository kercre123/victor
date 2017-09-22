/**
* File: behaviorTurnToFace.cpp
*
* Author: Kevin M. Karol
* Created: 6/6/17
*
* Description: Simple behavior to turn toward a face - face can either be passed
* in as part of IsRunnable, or selected using internal criteria using IsRunnable
* with a robot
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnToFace::BehaviorTurnToFace(const Json::Value& config)
: IBehavior(config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurnToFace::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  Pose3d wastedPose;
  TimeStamp_t lastTimeObserved = behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(wastedPose);
  std::set<Vision::FaceID_t> facesObserved = behaviorExternalInterface.GetFaceWorld().GetFaceIDsObservedSince(lastTimeObserved);
  if(facesObserved.size() > 0){
    _targetFace = behaviorExternalInterface.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
  }
  
  return _targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorTurnToFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_targetFace.IsValid()){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    StartActing(new TurnTowardsFaceAction(robot, _targetFace));
    return Result::RESULT_OK;
  }else{
    return RESULT_FAIL;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnToFace::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetFace.Reset();
}

  
} // namespace Cozmo
} // namespace Anki
