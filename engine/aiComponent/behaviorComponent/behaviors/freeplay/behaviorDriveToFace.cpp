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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorDriveToFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

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
BehaviorDriveToFace::BehaviorDriveToFace(const Json::Value& config)
: ICozmoBehavior(config)
, _currentState(State::TurnTowardsFace)
, _timeCancelTracking_s(0)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  Pose3d facePose;
  const TimeStamp_t timeLastFaceObserved = behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = robot.IsPoseInWorldOrigin(facePose);
  if(lastFaceInCurrentOrigin){
    const auto facesObserved = behaviorExternalInterface.GetFaceWorld().GetFaceIDsObservedSince(timeLastFaceObserved);
    if(facesObserved.size() > 0){
      _targetFace = behaviorExternalInterface.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
    }else{
      _targetFace.Reset();
    }
  }
  
  return _targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveToFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_targetFace.IsValid()){
    TransitionToTurningTowardsFace(behaviorExternalInterface);
    return Result::RESULT_OK;
  }else{
    PRINT_NAMED_WARNING("BehaviorDriveToFace.InitInternal.NoValidFace",
                        "Attempted to init behavior without a vaild face to drive to");
    return Result::RESULT_FAIL;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorDriveToFace::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_currentState == State::TrackFace &&
     _timeCancelTracking_s < currentTime_s){
    CancelDelegates(false);
    return Status::Complete;
  }
  
  return base::UpdateInternal_WhileRunning(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetFace.Reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTurningTowardsFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(TurnTowardsFace);
  const Vision::TrackedFace* facePtr = behaviorExternalInterface.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    CompoundActionSequential* turnAndVerifyAction = new CompoundActionSequential();
    turnAndVerifyAction->AddAction(new TurnTowardsFaceAction(_targetFace));
    turnAndVerifyAction->AddAction(new VisuallyVerifyFaceAction(facePtr->GetID()));
    
    DelegateIfInControl(new TurnTowardsFaceAction(_targetFace),
                [this, &behaviorExternalInterface, &facePtr](ActionResult result){
                  if(result == ActionResult::SUCCESS){
                    if(IsCozmoAlreadyCloseEnoughToFace(behaviorExternalInterface, facePtr->GetID())){
                      TransitionToAlreadyCloseEnough(behaviorExternalInterface);
                    }else{
                      TransitionToDrivingToFace(behaviorExternalInterface);
                    }
                  }
                });
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(DriveToFace);
  float distToHead;
  const Vision::TrackedFace* facePtr = behaviorExternalInterface.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr &&
     CalculateDistanceToFace(behaviorExternalInterface, facePtr->GetID(), distToHead)){
    DriveStraightAction* driveAction = new DriveStraightAction(distToHead - kMinDriveToFaceDistance_mm,
                                                               MAX_WHEEL_SPEED_MMPS);
    driveAction->SetDecel(DEFAULT_PATH_MOTION_PROFILE.decel_mmps2/kArbitraryDecelFactor);
    DelegateIfInControl(driveAction, &BehaviorDriveToFace::TransitionToTrackingFace);
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToAlreadyCloseEnough(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(AlreadyCloseEnough);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ComeHere_AlreadyHere),
              &BehaviorDriveToFace::TransitionToTrackingFace);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(TrackFace);
  _timeCancelTracking_s = kTimeUntilCancelFaceTrack_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::TrackedFace* facePtr = behaviorExternalInterface.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    const Vision::FaceID_t faceID = facePtr->GetID();
    DelegateIfInControl(new TrackFaceAction(faceID));
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsCozmoAlreadyCloseEnoughToFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID)
{
  float distToHead;
  if(CalculateDistanceToFace(behaviorExternalInterface, faceID, distToHead)){
    return distToHead <= kMinDriveToFaceDistance_mm;
  }
  
  return false;
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::CalculateDistanceToFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID, float& distance)
{
  // Get the distance between the robot and the head's pose on the X/Y plane
  const Vision::TrackedFace* facePtr = behaviorExternalInterface.GetFaceWorld().GetFace(_targetFace);
  if(facePtr != nullptr){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    
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
