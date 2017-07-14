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

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/trackFaceAction.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::BehaviorDriveToFace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _currentState(State::TurnTowardsFace)
, _timeCancelTracking_s(0)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();

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
  
  return _targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData ) const
{
  
  auto desiredTargets = preReqData.GetDesiredTargets();
  const Robot& robot = preReqData.GetRobot();
  if(ANKI_VERIFY(desiredTargets.size() == 1,
                 "BehaviorDriveToFace.IsRunnableInternal.IncorrectTagets",
                 "Recieved pre req with %zu targets",
                 desiredTargets.size())){
    _targetFace = robot.GetFaceWorld().GetSmartFaceID(*desiredTargets.begin());
    return _targetFace.IsValid();
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveToFace::InitInternal(Robot& robot)
{
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
void BehaviorDriveToFace::StopInternal(Robot& robot)
{
  _targetFace.Reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTurningTowardsFace(Robot& robot)
{
  SET_STATE(TurnTowardsFace);
  const Vision::TrackedFace* facePtr = robot.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    CompoundActionSequential* turnAndVerifyAction = new CompoundActionSequential(robot);
    turnAndVerifyAction->AddAction(new TurnTowardsFaceAction(robot, _targetFace));
    turnAndVerifyAction->AddAction(new VisuallyVerifyFaceAction(robot, facePtr->GetID()));
    
    StartActing(new TurnTowardsFaceAction(robot, _targetFace),
                [this, &robot, &facePtr](ActionResult result){
                  if(result == ActionResult::SUCCESS){
                    if(IsCozmoAlreadyCloseEnoughToFace(robot, facePtr->GetID())){
                      TransitionToAlreadyCloseEnough(robot);
                    }else{
                      TransitionToDrivingToFace(robot);
                    }
                  }
                });
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace(Robot& robot)
{
  SET_STATE(DriveToFace);
  float distToHead;
  const Vision::TrackedFace* facePtr = robot.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr &&
     CalculateDistanceToFace(robot, facePtr->GetID(), distToHead)){
    DriveStraightAction* driveAction = new DriveStraightAction(robot,
                                                               distToHead - kMinDriveToFaceDistance_mm,
                                                               MAX_WHEEL_SPEED_MMPS);
    driveAction->SetDecel(DEFAULT_PATH_MOTION_PROFILE.decel_mmps2/kArbitraryDecelFactor);
    StartActing(driveAction, &BehaviorDriveToFace::TransitionToTrackingFace);
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToAlreadyCloseEnough(Robot& robot)
{
  SET_STATE(AlreadyCloseEnough);
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ComeHere_AlreadyHere),
              &BehaviorDriveToFace::TransitionToTrackingFace);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace(Robot& robot)
{
  SET_STATE(TrackFace);
  _timeCancelTracking_s = kTimeUntilCancelFaceTrack_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::TrackedFace* facePtr = robot.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    const Vision::FaceID_t faceID = facePtr->GetID();
    StartActing(new TrackFaceAction(robot, faceID));
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsCozmoAlreadyCloseEnoughToFace(Robot& robot, Vision::FaceID_t faceID)
{
  float distToHead;
  if(CalculateDistanceToFace(robot, faceID, distToHead)){
    return distToHead <= kMinDriveToFaceDistance_mm;
  }
  
  return false;
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::CalculateDistanceToFace(Robot& robot, Vision::FaceID_t faceID, float& distance)
{
  // Get the distance between the robot and the head's pose on the X/Y plane
  const Vision::TrackedFace* facePtr = robot.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    Pose3d headPoseModified = facePtr->GetHeadPose();
    headPoseModified.SetTranslation({headPoseModified.GetTranslation().x(),
      headPoseModified.GetTranslation().y(),
      robot.GetPose().GetTranslation().z()});
    return ComputeDistanceBetween(headPoseModified, robot.GetPose(), distance);
  }
  
  return false;
}

  

  
}
}
