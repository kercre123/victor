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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

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
bool BehaviorTurnToFace::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  Pose3d wastedPose;
  TimeStamp_t lastTimeObserved = robot.GetFaceWorld().GetLastObservedFace(wastedPose);
  std::set<Vision::FaceID_t> facesObserved = robot.GetFaceWorld().GetFaceIDsObservedSince(lastTimeObserved);
  if(facesObserved.size() > 0){
    _targetFace = robot.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
  }
  
  return _targetFace.IsValid();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurnToFace::IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData) const
{
  if(ANKI_VERIFY(preReqData.GetDesiredTargets().size() == 1,
                 "BehaviorTurnToFace.IsRunnableInternal.PreReqAcknowledgeFace",
                 "Received %zu faces", preReqData.GetDesiredTargets().size())){
    const Robot& robot = preReqData.GetRobot();
    _targetFace = robot.GetFaceWorld().GetSmartFaceID(*preReqData.GetDesiredTargets().begin());
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
