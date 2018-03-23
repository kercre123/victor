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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {
  
namespace{
// We want cozmo to slow down as he approaches the face since there's probably
// an edge.  The decel factor relative to the default profile is arbitrary but
// tuned based off some testing to attempt the right feeling of "approaching" the face
const float kArbitraryDecelFactor = 3.0f;
  
const char* const kTimeUntilCancelFaceTrackKey = "timeUntilCancelFaceTrack_s";
const char* const kMinDriveToFaceDistanceKey = "minDriveToFaceDistanceKey_mm";
const char* const kDebugName = "BehaviorDriveToFace";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::DynamicVariables::DynamicVariables()
{
  timeCancelTracking_s = 0;
  currentState         = State::TurnTowardsFace;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::BehaviorDriveToFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.timeUntilCancelFaceTrack_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceTrackKey, kDebugName );
  _iConfig.minDriveToFaceDistance_mm = JsonTools::ParseFloat( config, kMinDriveToFaceDistanceKey, kDebugName );
  
  MakeMemberTunable( _iConfig.timeUntilCancelFaceTrack_s, kTimeUntilCancelFaceTrackKey, kDebugName );
  MakeMemberTunable( _iConfig.minDriveToFaceDistance_mm, kMinDriveToFaceDistanceKey, kDebugName );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTimeUntilCancelFaceTrackKey,
    kMinDriveToFaceDistanceKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  Pose3d facePose;
  const TimeStamp_t timeLastFaceObserved = GetBEI().GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = robotInfo.IsPoseInWorldOrigin(facePose);
  if(lastFaceInCurrentOrigin){
    const auto facesObserved = GetBEI().GetFaceWorld().GetFaceIDsObservedSince(timeLastFaceObserved);
    if(facesObserved.size() > 0){
      _dVars.targetFace = GetBEI().GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
    }else{
      _dVars.targetFace.Reset();
    }
  }
  
  return _dVars.targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorActivated()
{
  if(_dVars.targetFace.IsValid()){
    TransitionToTurningTowardsFace();
    
  }else{
    PRINT_NAMED_WARNING("BehaviorDriveToFace.InitInternal.NoValidFace",
                        "Attempted to init behavior without a vaild face to drive to");
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_dVars.currentState == State::TrackFace &&
     _dVars.timeCancelTracking_s < currentTime_s){
    CancelSelf();
  }  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorDeactivated()
{
  _dVars.targetFace.Reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTurningTowardsFace()
{
  SET_STATE(TurnTowardsFace);
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr){
    CompoundActionSequential* turnAndVerifyAction = new CompoundActionSequential();
    turnAndVerifyAction->AddAction(new TurnTowardsFaceAction(_dVars.targetFace));
    turnAndVerifyAction->AddAction(new VisuallyVerifyFaceAction(facePtr->GetID()));
    
    DelegateIfInControl(new TurnTowardsFaceAction(_dVars.targetFace),
                [this, facePtr](ActionResult result){
                  if(result == ActionResult::SUCCESS){
                    if(IsCozmoAlreadyCloseEnoughToFace(facePtr->GetID())){
                      TransitionToAlreadyCloseEnough();
                    }else{
                      TransitionToDrivingToFace();
                    }
                  }
                });
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace()
{
  SET_STATE(DriveToFace);
  float distToHead;
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr &&
     CalculateDistanceToFace(facePtr->GetID(), distToHead)){
    DriveStraightAction* driveAction = new DriveStraightAction(distToHead - _iConfig.minDriveToFaceDistance_mm,
                                                               MAX_WHEEL_SPEED_MMPS);
    driveAction->SetDecel(DEFAULT_PATH_MOTION_PROFILE.decel_mmps2/kArbitraryDecelFactor);
    DelegateIfInControl(driveAction, &BehaviorDriveToFace::TransitionToTrackingFace);
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToAlreadyCloseEnough()
{
  SET_STATE(AlreadyCloseEnough);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::AlreadyAtFace),
              &BehaviorDriveToFace::TransitionToTrackingFace);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace()
{
  SET_STATE(TrackFace);
  _dVars.timeCancelTracking_s = _iConfig.timeUntilCancelFaceTrack_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr){
    const Vision::FaceID_t faceID = facePtr->GetID();
    DelegateIfInControl(new TrackFaceAction(faceID));
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::SetState_internal(State state, const std::string& stateName)
{
  _dVars.currentState = state;
  SetDebugStateName(stateName);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::IsCozmoAlreadyCloseEnoughToFace(Vision::FaceID_t faceID)
{
  float distToHead;
  if(CalculateDistanceToFace(faceID, distToHead)){
    return distToHead <= _iConfig.minDriveToFaceDistance_mm;
  }
  
  return false;
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::CalculateDistanceToFace(Vision::FaceID_t faceID, float& distance)
{
  // Get the distance between the robot and the head's pose on the X/Y plane
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr){
    const auto& robotInfo = GetBEI().GetRobotInfo();

    
    Pose3d headPoseModified = facePtr->GetHeadPose();
    headPoseModified.SetTranslation({headPoseModified.GetTranslation().x(),
      headPoseModified.GetTranslation().y(),
      robotInfo.GetPose().GetTranslation().z()});
    return ComputeDistanceBetween(headPoseModified, robotInfo.GetPose(), distance);
  }
  
  return false;
}

  

  
}
}
