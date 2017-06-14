/**
* File: behaviorDriveToFace.cpp
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that turns towards and then drives to a face
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorDriveToFace.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/trackFaceAction.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {
  
namespace{
CONSOLE_VAR(f32, kTimeUntilCancelFaceTrack_s,"BehaviorDriveToFace", 5.0f);
CONSOLE_VAR(f32, kMinDriveToFaceDistance_mm, "BehaviorDriveToFace", 200.0f);
// We want cozmo to slow down as he approaches the face since there's probably
// an edge.  The decel factor relative to the default profile is arbitrary but
// tuned based off some testing to attempt the right feeling of "approaching" the face
const float kArbitraryDecelFactor = 3.0f;
}
   
  
BehaviorDriveToFace::BehaviorDriveToFace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _currentState(State::TurnTowardsFace)
, _timeCancelTracking_s(0)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  Pose3d facePose;
  const Robot& robot = preReqData.GetRobot();
  robot.GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = &facePose.FindOrigin() == robot.GetWorldOrigin();
  
  return lastFaceInCurrentOrigin;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveToFace::InitInternal(Robot& robot)
{
  Pose3d facePose;
  const TimeStamp_t timeLastFaceObserved = robot.GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = &facePose.FindOrigin() == robot.GetWorldOrigin();
  if(lastFaceInCurrentOrigin){
    const auto facesObserved = robot.GetFaceWorld().GetFaceIDsObservedSince(timeLastFaceObserved);
    if(facesObserved.size() > 0){
      _targetFace = robot.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
    }else{
      _targetFace.Reset();
    }
  }
  
  if(_targetFace.IsValid()){
    TransitionToTurningTowardsFace(robot);
    return Result::RESULT_OK;
  }else{
    PRINT_NAMED_WARNING("BehaviorDriveToFace.InitInternal.NoValidFace",
                        "Attempted to init behavior without a vaild face to drive to");
    return Result::RESULT_FAIL;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorDriveToFace::UpdateInternal(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_currentState == State::TrackFace &&
     _timeCancelTracking_s < currentTime_s){
    StopActing(false);
    return Status::Complete;
  }
  
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTurningTowardsFace(Robot& robot)
{
  SET_STATE(TurnTowardsFace);
  CompoundActionSequential* turnAndVerifyAction = new CompoundActionSequential(robot);
  turnAndVerifyAction->AddAction(new TurnTowardsFaceAction(robot, _targetFace));
  turnAndVerifyAction->AddAction(new VisuallyVerifyFaceAction(
                    robot, robot.GetFaceWorld().GetFace(_targetFace)->GetID()));

  
  StartActing(new TurnTowardsFaceAction(robot, _targetFace),
              [this, &robot](ActionResult result){
                if(result == ActionResult::SUCCESS){
                  TransitionToDrivingToFace(robot);
                }
              });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace(Robot& robot)
{
  SET_STATE(DriveToFace);
  // Get the distance between the robot and the head's pose on the X/Y plane
  const Vision::TrackedFace* facePtr = robot.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    Pose3d headPoseModified = facePtr->GetHeadPose();
    headPoseModified.SetTranslation({headPoseModified.GetTranslation().x(),
                                     headPoseModified.GetTranslation().y(),
                                     robot.GetPose().GetTranslation().z()});
    f32 distToHead;
    if(ComputeDistanceBetween(headPoseModified, robot.GetPose(), distToHead) &&
       distToHead > kMinDriveToFaceDistance_mm){
      
      DriveStraightAction* driveAction = new DriveStraightAction(robot,
                                                                 distToHead - kMinDriveToFaceDistance_mm,
                                                                 MAX_WHEEL_SPEED_MMPS);
      driveAction->SetDecel(DEFAULT_PATH_MOTION_PROFILE.decel_mmps2/kArbitraryDecelFactor);
      StartActing(driveAction, &BehaviorDriveToFace::TransitionToTrackingFace);
    }else{
      TransitionToTrackingFace(robot);
    }
  }else{
    PRINT_CH_INFO("BehaviorDriveToFace.TransitionToDrivingToFace.NullFace",
                  "Target face appears to have disappeared",
                  "Face ID: %s", _targetFace.GetDebugStr().c_str());
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace(Robot& robot)
{
  SET_STATE(TrackFace);
  _timeCancelTracking_s = kTimeUntilCancelFaceTrack_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::FaceID_t faceID = robot.GetFaceWorld().GetFace(_targetFace)->GetID();
  StartActing(new TrackFaceAction(robot, faceID));
}
  
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}
  
}
}
