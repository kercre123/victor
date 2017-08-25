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

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"

#include "engine/actions/basicActions.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnToFace::BehaviorTurnToFace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurnToFace::IsRunnableInternal(const Robot& robot) const
{
  Pose3d wastedPose;
  TimeStamp_t lastTimeObserved = robot.GetFaceWorld().GetLastObservedFace(wastedPose);
  std::set<Vision::FaceID_t> facesObserved = robot.GetFaceWorld().GetFaceIDsObservedSince(lastTimeObserved);
  if(facesObserved.size() > 0){
    _targetFace = robot.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
  }
  
  return _targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorTurnToFace::InitInternal(Robot& robot)
{
  if(_targetFace.IsValid()){
    StartActing(new TurnTowardsFaceAction(robot, _targetFace));
    return Result::RESULT_OK;
  }else{
    return RESULT_FAIL;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnToFace::StopInternal(Robot& robot)
{
  _targetFace.Reset();
}

  
} // namespace Cozmo
} // namespace Anki
