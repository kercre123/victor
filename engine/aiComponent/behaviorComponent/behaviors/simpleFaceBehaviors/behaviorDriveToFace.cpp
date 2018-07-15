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
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
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
  
const char* const kTimeUntilCancelFaceTrackKey   = "timeUntilCancelFaceTrack_s";
const char* const kMinDriveToFaceDistanceKey     = "minDriveToFaceDistanceKey_mm";
const char* const kTrackFaceOnceKnownKey         = "trackFaceOnceKnown";
const char* const kAnimTooCloseKey               = "animTooClose";
const char* const kAnimDriveOverrideKey          = "animDriveOverride";
const char* const kMotionProfileKey              = "motionProfile";
const char* const kInitialAnimKey                = "initialAnimation";
const char* const kSuccessAnimKey                = "animationAfterDrive";
const char* const kDebugName = "BehaviorDriveToFace";

static constexpr const float kMaxTimeToTrackWithoutFace_s = 1.0f;

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
  else if( config.isMember(kTimeUntilCancelFaceTrackKey) ) {
    PRINT_NAMED_WARNING("BehaviorDriveToFace.InvalidConfig.UselessKey",
                        "%s: Key '%s' specified, but no tracking ('%s' not true). Will be ignored",
                        GetDebugLabel().c_str(),
                        kTimeUntilCancelFaceTrackKey,
                        kTrackFaceOnceKnownKey);
  }
  
  JsonTools::GetCladEnumFromJSON(config, kAnimTooCloseKey, _iConfig.animTooClose, kDebugName);
  _iConfig.animDriveOverride = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimDriveOverrideKey, _iConfig.animDriveOverride, kDebugName, false);
  _iConfig.initialAnim = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kInitialAnimKey, _iConfig.initialAnim, kDebugName, false);
  _iConfig.successAnim = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kSuccessAnimKey, _iConfig.successAnim, kDebugName, false);

  if( _iConfig.animDriveOverride != AnimationTrigger::Count &&
      ( _iConfig.initialAnim != AnimationTrigger::Count ||
        _iConfig.successAnim != AnimationTrigger::Count ) ) {
    PRINT_NAMED_WARNING("BehaviorDriveToFace.InvalidConfig.AnimConflict",
                        "Behavior '%s' specified json key '%s' and at least one of '%s' or '%s', which won't have effect",
                        GetDebugLabel().c_str(),
                        kAnimDriveOverrideKey,
                        kInitialAnimKey,
                        kSuccessAnimKey);
  }


  if( !config[kMotionProfileKey].isNull() ) {
    _iConfig.customMotionProfile = std::make_unique<PathMotionProfile>();
    _iConfig.customMotionProfile->SetFromJSON( config[kMotionProfileKey] );
  }
  
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
    kMotionProfileKey,
    kInitialAnimKey,
    kSuccessAnimKey,
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
  
  if( _iConfig.customMotionProfile != nullptr ) {
    SmartSetMotionProfile( *_iConfig.customMotionProfile );
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
    case State::FinalAnim:
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

    auto* action = new CompoundActionSequential;

    if( _iConfig.initialAnim != AnimationTrigger::Count ) {
      action->AddAction(new TriggerAnimationAction( _iConfig.initialAnim));
    }

    // set pose now (rather than when anim finishes) in case the anim turns the robot
    Pose3d pose = GetBEI().GetRobotInfo().GetPose();
    pose.TranslateForward( distToHead - _iConfig.minDriveToFaceDistance_mm );
    action->AddAction(new DriveToPoseAction( pose, true ));
    CancelDelegates(false);
    DelegateIfInControl(action, [this](ActionResult res) {
        if( res == ActionResult::SUCCESS ) {
          TransitionToPlayingFinalAnim();
        }
        // else let the behavior end now
      });

  } else if( _iConfig.animDriveOverride != AnimationTrigger::Count ) {
    auto* action = new TriggerAnimationAction( _iConfig.animDriveOverride );
    CancelDelegates(false);
    DelegateIfInControl(action, &BehaviorDriveToFace::TransitionToTrackingFace);
  } else {
    TransitionToAlreadyCloseEnough();
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToPlayingFinalAnim()
{
  SET_STATE(FinalAnim);

  if( _iConfig.successAnim != AnimationTrigger::Count ) {
    DelegateIfInControl(new TriggerAnimationAction(_iConfig.successAnim), &BehaviorDriveToFace::TransitionToTrackingFace);
  }
  else {
    TransitionToTrackingFace();
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

  if( !_iConfig.trackFaceOnceKnown ) {
    // no tracking requested, just stop now
    return;
  }

  SET_STATE(TrackFace);

  _dVars.trackEndTime_s = 0;

  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.trackEndTime_s = _iConfig.timeUntilCancelTracking_s + currTime_s;
    const Vision::FaceID_t faceID = facePtr->GetID();
    CancelDelegates(false);
    auto* trackAction = new TrackFaceAction(faceID);
    trackAction->SetUpdateTimeout(kMaxTimeToTrackWithoutFace_s);
    DelegateIfInControl(trackAction);
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
