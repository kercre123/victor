/**
* File: behaviorDriveToFace.cpp
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that drives to a face
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorDriveToFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {
  
namespace{
// We want cozmo to slow down as he approaches the face since there's probably
// an edge.  The decel factor relative to the default profile is arbitrary but
// tuned based off some testing to attempt the right feeling of "approaching" the face
const float kArbitraryDecelFactor = 3.0f;
  
const char* const kTimeUntilCancelFaceTrackKey   = "timeUntilCancelFaceTrack_s";
const char* const kMinDriveToFaceDistanceKey     = "minDriveToFaceDistanceKey_mm";
const char* const kTrackFaceOnceKnownKey         = "trackFaceOnceKnown";
const char* const kAnimTooCloseKey               = "animTooClose";
const char* const kAnimDriveOverrideKey          = "animDriveOverride";
const char* const kDebugName = "BehaviorDriveToFace";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::DynamicVariables::DynamicVariables()
{
  trackEndTime_s         = 0;
  currentState           = State::Invalid;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::BehaviorDriveToFace(const Json::Value& config)
  : ISimpleFaceBehavior(config)
{
  _iConfig.minDriveToFaceDistance_mm = JsonTools::ParseFloat( config, kMinDriveToFaceDistanceKey, kDebugName );
  
  _iConfig.trackFaceOnceKnown = config.get( kTrackFaceOnceKnownKey, true ).asBool();
  if( _iConfig.trackFaceOnceKnown ) {
    _iConfig.timeUntilCancelTracking_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceTrackKey, kDebugName );
  }
  
  JsonTools::GetCladEnumFromJSON(config, kAnimTooCloseKey, _iConfig.animTooClose, kDebugName);
  _iConfig.animDriveOverride = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimDriveOverrideKey, _iConfig.animDriveOverride, kDebugName, false);
  
  MakeMemberTunable( _iConfig.timeUntilCancelTracking_s, kTimeUntilCancelFaceTrackKey, kDebugName );
  MakeMemberTunable( _iConfig.minDriveToFaceDistance_mm, kMinDriveToFaceDistanceKey, kDebugName );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
  modifiers.behaviorAlwaysDelegates = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMinDriveToFaceDistanceKey,
    kTrackFaceOnceKnownKey,
    kTimeUntilCancelFaceTrackKey,
    kAnimTooCloseKey,
    kAnimDriveOverrideKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::WantsToBeActivatedBehavior() const
{
  return GetTargetFace().IsValid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    Vision::FaceID_t faceID = facePtr->GetID();
    if( IsRobotAlreadyCloseEnoughToFace( faceID ) ) {
      TransitionToAlreadyCloseEnough();
    } else {
      TransitionToDrivingToFace();
    }
  } else {
    TransitionToAlreadyCloseEnough();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch( _dVars.currentState ) {
    case State::TrackFace:
    {
      if( _dVars.trackEndTime_s < currentTime_s ) {
        // done tracking face
        CancelSelf();
      }
    }
      break;
    case State::Invalid:
    case State::DriveToFace:
    case State::AlreadyCloseEnough:
      // nothing to do, or waiting for action/behavior callback
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace()
{
  SET_STATE(DriveToFace);
  float distToHead;
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr &&
     CalculateDistanceToFace(facePtr->GetID(), distToHead) &&
     _iConfig.animDriveOverride == AnimationTrigger::Count )
  {
    DriveStraightAction* driveAction = new DriveStraightAction(distToHead - _iConfig.minDriveToFaceDistance_mm,
                                                               0.5*MAX_SAFE_WHEEL_SPEED_MMPS);
    driveAction->SetDecel(DEFAULT_PATH_MOTION_PROFILE.decel_mmps2/kArbitraryDecelFactor);
    CancelDelegates(false);
    DelegateIfInControl(driveAction, [this]() {
      if( _iConfig.trackFaceOnceKnown ) {
        TransitionToTrackingFace();
      }
    });
  } else if( _iConfig.animDriveOverride != AnimationTrigger::Count ) {
    auto* action = new TriggerAnimationAction( _iConfig.animDriveOverride );
    CancelDelegates(false);
    DelegateIfInControl(action, [this]() {
      if( _iConfig.trackFaceOnceKnown ) {
        TransitionToTrackingFace();
      }
    });
  } else {
    TransitionToAlreadyCloseEnough();
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToAlreadyCloseEnough()
{
  SET_STATE(AlreadyCloseEnough);
  auto* action = new TriggerAnimationAction( _iConfig.animTooClose );
  CancelDelegates(false);
  DelegateIfInControl(action, [this]() {
    if( _iConfig.trackFaceOnceKnown ) {
      TransitionToTrackingFace();
    }
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace()
{
  SET_STATE(TrackFace);
  _dVars.trackEndTime_s = 0;
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    _dVars.trackEndTime_s = _iConfig.timeUntilCancelTracking_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const Vision::FaceID_t faceID = facePtr->GetID();
    CancelDelegates(false);
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
bool BehaviorDriveToFace::IsRobotAlreadyCloseEnoughToFace(Vision::FaceID_t faceID)
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
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
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
