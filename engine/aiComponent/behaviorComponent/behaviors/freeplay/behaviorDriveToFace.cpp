/**
* File: behaviorDriveToFace.cpp
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that turns towards and then drives to a face. If no face is known or found, it optionally tries to find it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorDriveToFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
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
  
const char* const kTimeUntilCancelFaceLookingKey = "timeUntilCancelFaceLooking_s";
const char* const kTimeUntilCancelFaceSearchKey  = "timeUntilCancelFaceSearch_s";
const char* const kTimeUntilCancelFaceTrackKey   = "timeUntilCancelFaceTrack_s";
const char* const kMinDriveToFaceDistanceKey     = "minDriveToFaceDistanceKey_mm";
const char* const kStartsWithMicDirectionKey     = "startsWithMicDirection";
const char* const kSearchForFaceBehaviorKey      = "searchForFaceBehavior";
const char* const kTrackFaceOnceKnownKey         = "trackFaceOnceKnown";
const char* const kAlwaysDetectFacesKey          = "alwaysDetectFaces";
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
  stateEndTime_s         = 0;
  currentState           = State::Invalid;
  lastFaceTimeStamp_ms   = 0;
  activationTimeStamp_ms = 0;
  targetFace.Reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveToFace::BehaviorDriveToFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.timeUntilCancelFaceLooking_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceLookingKey, kDebugName );
  
  _iConfig.minDriveToFaceDistance_mm = JsonTools::ParseFloat( config, kMinDriveToFaceDistanceKey, kDebugName );
  
  _iConfig.startedWithMicDirection = config.get( kStartsWithMicDirectionKey, false ).asBool();
  
  _iConfig.trackFaceOnceKnown = config.get( kTrackFaceOnceKnownKey, true ).asBool();
  if( _iConfig.trackFaceOnceKnown ) {
    _iConfig.timeUntilCancelTracking_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceTrackKey, kDebugName );
  }
  
  
  _iConfig.alwaysDetectFaces = config.get( kAlwaysDetectFacesKey, false ).asBool();
  
  _iConfig.searchBehaviorID = config.get( kSearchForFaceBehaviorKey, "" ).asString();
  if( !_iConfig.searchBehaviorID.empty() ) {
    _iConfig.timeUntilCancelSearching_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceSearchKey, kDebugName );
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
  if( _iConfig.alwaysDetectFaces ) {
    // always look for faces, so that when the robot turns in the direction of a voice command, hopefully a
    // face pose is already known
    modifiers.visionModesForActivatableScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::Low} );
  }
  
  modifiers.behaviorAlwaysDelegates = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMinDriveToFaceDistanceKey,
    kStartsWithMicDirectionKey,
    kTrackFaceOnceKnownKey,
    kTimeUntilCancelFaceLookingKey,
    kTimeUntilCancelFaceSearchKey,
    kTimeUntilCancelFaceTrackKey,
    kSearchForFaceBehaviorKey,
    kAlwaysDetectFacesKey,
    kAnimTooCloseKey,
    kAnimDriveOverrideKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.searchFaceBehavior != nullptr ) {
    delegates.insert( _iConfig.searchFaceBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::InitBehavior()
{
  ICozmoBehaviorPtr behavior = nullptr;
  if( !_iConfig.searchBehaviorID.empty() ) {
    behavior = FindBehavior( _iConfig.searchBehaviorID );
  }
  _iConfig.searchFaceBehavior = behavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  _dVars.activationTimeStamp_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  _dVars.lastFaceTimeStamp_ms = _dVars.activationTimeStamp_ms;
  
  const bool hasRecentFace = GetRecentFace( _dVars.targetFace, _dVars.lastFaceTimeStamp_ms );
  if( _iConfig.startedWithMicDirection ) {
    TransitionToLookingInMicDirection();
  } else if( hasRecentFace ) {
    TransitionToTurningTowardsFace();
  } else {
    // start looking in the current direction
    TransitionToFindingFaceInCurrentDirection();
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
    case State::LookForFaceInMicDirection:
    {
      if( currentTime_s < _dVars.stateEndTime_s ) {
        SmartFaceID newFace;
        TimeStamp_t timeStamp_ms = 0;
        const bool hasFace = GetRecentFaceSince( _dVars.activationTimeStamp_ms, newFace, timeStamp_ms );
        const bool differentOrNew = (newFace != _dVars.targetFace) || (timeStamp_ms > _dVars.lastFaceTimeStamp_ms);
        if( hasFace && differentOrNew ) {
          // a new face was spotted in the direction it's facing, or the same face was spotted in that direction again
          _dVars.targetFace = newFace;
          _dVars.lastFaceTimeStamp_ms = timeStamp_ms;
          TransitionToTurningTowardsFace();
        }
      } else if( _dVars.targetFace.IsValid() ) {
        // timeout in the mic direction, but the original face is still valid, turn back towards it.
        // we could potentially only do this if the timestamp is recent, but it still might look cool if it
        // looks in the direction it last saw a face
        TransitionToTurningTowardsFace();
      } else if( _iConfig.searchFaceBehavior != nullptr ) {
        // no face is known at this point, and we already spent time looking in the current direction
        TransitionToSearchingForFace();
      } else {
        // give up
        CancelSelf();
      }
    }
      break;
      
    case State::FindFaceInCurrentDirection:
    case State::SearchForFace:
    {
      if( currentTime_s < _dVars.stateEndTime_s ) {
        SmartFaceID newFace;
        TimeStamp_t timeStamp_ms = 0;
        const bool hasFace = GetRecentFace( newFace, timeStamp_ms );
        const bool differentOrNew = (newFace != _dVars.targetFace) || (timeStamp_ms > _dVars.lastFaceTimeStamp_ms);
        if( hasFace && differentOrNew ) {
          // a new face was spotted in the direction it's facing
          _dVars.targetFace = newFace;
          TransitionToTurningTowardsFace();
        }
      } else if( (_dVars.currentState == State::FindFaceInCurrentDirection) && (_iConfig.searchFaceBehavior != nullptr) ) {
        // no face is known at this point. look for one if config added a search behavior
        TransitionToSearchingForFace();
      } else {
        // can't find or verify a face position. give up
        CancelSelf();
      }
    }
      break;
    case State::TrackFace:
    {
      if( _dVars.stateEndTime_s < currentTime_s ) {
        // done tracking face
        CancelSelf();
      }
    }
      break;
    case State::Invalid:
    case State::TurnTowardsPreviousFace:
    case State::DriveToFace:
    case State::AlreadyCloseEnough:
      // nothing to do, or waiting for action/behavior callback
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::OnBehaviorDeactivated()
{
  _dVars.targetFace.Reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToLookingInMicDirection()
{
  // not doing anything here, just waiting for a face or timeout
  SET_STATE(LookForFaceInMicDirection);
  _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelFaceLooking_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTurningTowardsFace()
{
  SET_STATE(TurnTowardsPreviousFace);
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr){
    Vision::FaceID_t faceID = facePtr->GetID();
    auto* action = new TurnTowardsFaceAction( _dVars.targetFace );
    action->SetLockOnClosestFaceAfterTurn( true ); // accept any face once turning in the direction of targetFace
    action->SetRequireFaceConfirmation( true );
    auto callback = [this, faceID]( ActionResult result ){
      if( result == ActionResult::SUCCESS ) {
        if( IsRobotAlreadyCloseEnoughToFace( faceID ) ) {
          TransitionToAlreadyCloseEnough();
        } else {
          TransitionToDrivingToFace();
        }
      } else {
        TransitionToFindingFaceInCurrentDirection();
      }
    };
    CancelDelegates(false);
    DelegateIfInControl(action, callback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToFindingFaceInCurrentDirection()
{
  // not doing anything here, just waiting for a face or timeout
  SET_STATE( FindFaceInCurrentDirection );
  _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelFaceLooking_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToSearchingForFace()
{
  SET_STATE( SearchForFace );
  _dVars.stateEndTime_s = 0;
  if( _iConfig.searchFaceBehavior != nullptr ) {
    _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelSearching_s;
    
    CancelDelegates(false);
    if( _iConfig.searchFaceBehavior->WantsToBeActivated() ) {
      DelegateIfInControl( _iConfig.searchFaceBehavior.get() );
    } else {
      PRINT_NAMED_WARNING("BehaviorDriveToFace.TransitionToSearchingForFace.SearchDoesntWantToActivate",
                          "Search behavior %s doesn't want to activate", _iConfig.searchBehaviorID.c_str() );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToDrivingToFace()
{
  SET_STATE(DriveToFace);
  float distToHead;
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
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
      } else {
        // done
        CancelSelf();
      }
    });
  } else if( _iConfig.animDriveOverride != AnimationTrigger::Count ) {
    auto* action = new TriggerAnimationAction( _iConfig.animDriveOverride );
    CancelDelegates(false);
    DelegateIfInControl(action, [this]() {
      if( _iConfig.trackFaceOnceKnown ) {
        TransitionToTrackingFace();
      } else {
        // done
        CancelSelf();
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
    } else {
      // done
      CancelSelf();
    }
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveToFace::TransitionToTrackingFace()
{
  SET_STATE(TrackFace);
  _dVars.stateEndTime_s = 0;
  // Runs forever - the behavior will be stopped in the Update loop when the
  // timeout for face tracking is hit
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr != nullptr){
    _dVars.stateEndTime_s = _iConfig.timeUntilCancelTracking_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const Vision::FaceID_t faceID = facePtr->GetID();
    CancelDelegates(false);
    DelegateIfInControl(new TrackFaceAction(faceID));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::GetRecentFace( SmartFaceID& faceID, TimeStamp_t& timeStamp_ms )
{
  return GetRecentFaceSince( 0, faceID, timeStamp_ms );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveToFace::GetRecentFaceSince( TimeStamp_t sinceTime_ms, SmartFaceID& faceID, TimeStamp_t& timeStamp_ms )
{
  SmartFaceID retFace;
  retFace.Reset();
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  Pose3d facePose;
  TimeStamp_t timeLastFaceObserved = GetBEI().GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = robotInfo.IsPoseInWorldOrigin(facePose);
  if( lastFaceInCurrentOrigin ) {
    timeLastFaceObserved = Anki::Util::Max( sinceTime_ms, timeLastFaceObserved );
    const auto facesObserved = GetBEI().GetFaceWorld().GetFaceIDsObservedSince(timeLastFaceObserved);
    if( facesObserved.size() > 0 ) {
      retFace = GetBEI().GetFaceWorld().GetSmartFaceID( *facesObserved.begin() );
    }
  }
  

  if( retFace.IsValid() ) {
    faceID = retFace;
    timeStamp_ms = timeLastFaceObserved;
    return true;
  } else {
    return false;
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
