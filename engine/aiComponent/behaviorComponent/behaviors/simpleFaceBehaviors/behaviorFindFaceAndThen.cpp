/**
 * File: behaviorFindFaceAndThen.h
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: Finds a face either in the activation direction, or wherever one was last seen,
 *              and if it finds one, either delegates to a followup behavior or exits
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorFindFaceAndThen.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/iSimpleFaceBehavior.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {
  
namespace{
  
const char* const kTimeUntilCancelFaceLookingKey = "timeUntilCancelFaceLooking_s";
const char* const kTimeUntilCancelFaceSearchKey  = "timeUntilCancelFaceSearch_s";
const char* const kTimeUntilCancelFollowupKey    = "timeUntilCancelFollowup_s";
const char* const kStartsWithMicDirectionKey     = "startsWithMicDirection";
const char* const kShouldLeaveChargerFirstKey    = "shouldLeaveChargerFirst";
const char* const kDriveOffChargerBehaviorKey    = "driveOffChargerBehavior";
const char* const kSearchForFaceBehaviorKey      = "searchForFaceBehavior";
const char* const kAlwaysDetectFacesKey          = "alwaysDetectFaces";
const char* const kBehaviorOnceFoundKey          = "behavior";
const char* const kBehaviorIsSimpleFaceKey       = "callSetFaceOnBehavior";
const char* const kExitOnceFoundKey              = "exitOnceFound";
  
const char* const kDebugName = "BehaviorFindFaceAndThen";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaceAndThen::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaceAndThen::DynamicVariables::DynamicVariables()
{
  stateEndTime_s         = 0;
  currentState           = State::Invalid;
  lastFaceTimeStamp_ms   = 0;
  activationTimeStamp_ms = 0;
  targetFace.Reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaceAndThen::BehaviorFindFaceAndThen(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.timeUntilCancelFaceLooking_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceLookingKey, kDebugName );
  _iConfig.timeUntilCancelFollowup_s = config.get( kTimeUntilCancelFollowupKey, -1.0f ).asFloat();
  
  _iConfig.shouldLeaveChargerFirst = JsonTools::ParseBool( config, kShouldLeaveChargerFirstKey, kDebugName );
  if( _iConfig.shouldLeaveChargerFirst ) {
    _iConfig.driveOffChargerBehaviorID = JsonTools::ParseString( config, kDriveOffChargerBehaviorKey, kDebugName );
  }
  
  _iConfig.startedWithMicDirection = config.get( kStartsWithMicDirectionKey, true ).asBool();
  
  _iConfig.alwaysDetectFaces = config.get( kAlwaysDetectFacesKey, false ).asBool();
  
  _iConfig.searchBehaviorID = config.get( kSearchForFaceBehaviorKey, "" ).asString();
  if( !_iConfig.searchBehaviorID.empty() ) {
    _iConfig.timeUntilCancelSearching_s = JsonTools::ParseFloat( config, kTimeUntilCancelFaceSearchKey, kDebugName );
  }
  
  
  _iConfig.behaviorOnceFoundID = config.get( kBehaviorOnceFoundKey, "" ).asString();

  _iConfig.behaviorOnceFoundIsSimpleFace = config.get(kBehaviorIsSimpleFaceKey, true).asBool();
  _iConfig.exitOnceFound = config.get(kExitOnceFoundKey, false).asBool();
  
  ANKI_VERIFY( _iConfig.exitOnceFound == _iConfig.behaviorOnceFoundID.empty(),
               "BehaviorFindFaceAndThen.Ctor.InvalidBehavior",
               "A 'behavior' must be provided, or set 'exitOnceFound' to false" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
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
void BehaviorFindFaceAndThen::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kStartsWithMicDirectionKey,
    kShouldLeaveChargerFirstKey,
    kDriveOffChargerBehaviorKey,
    kTimeUntilCancelFaceLookingKey,
    kTimeUntilCancelFaceSearchKey,
    kTimeUntilCancelFollowupKey,
    kSearchForFaceBehaviorKey,
    kAlwaysDetectFacesKey,
    kBehaviorOnceFoundKey,
    kBehaviorIsSimpleFaceKey,
    kExitOnceFoundKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.searchFaceBehavior != nullptr ) {
    delegates.insert( _iConfig.searchFaceBehavior.get() );
  }
  if( _iConfig.driveOffChargerBehavior != nullptr ) {
    delegates.insert( _iConfig.driveOffChargerBehavior.get() );
  }
  if( _iConfig.behaviorOnceFound != nullptr ) {
    delegates.insert( _iConfig.behaviorOnceFound.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaceAndThen::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::InitBehavior()
{
  if( !_iConfig.searchBehaviorID.empty() ) {
    _iConfig.searchFaceBehavior = FindBehavior( _iConfig.searchBehaviorID );
  }
  if( !_iConfig.driveOffChargerBehaviorID.empty() ) {
    _iConfig.driveOffChargerBehavior = FindBehavior( _iConfig.driveOffChargerBehaviorID );
  }

  if( !_iConfig.behaviorOnceFoundID.empty() ) {
    _iConfig.behaviorOnceFound = FindBehavior( _iConfig.behaviorOnceFoundID );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  _dVars.activationTimeStamp_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  _dVars.lastFaceTimeStamp_ms = _dVars.activationTimeStamp_ms;
  
  const bool hasRecentFace = GetRecentFace( _dVars.targetFace, _dVars.lastFaceTimeStamp_ms );
  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( onCharger && _iConfig.shouldLeaveChargerFirst ) {
    TransitionToDrivingOffCharger();
  } else if( onCharger ) {
    TransitionToFindingFaceInCurrentDirection();
  } else if( _iConfig.startedWithMicDirection ) {
    TransitionToLookingInMicDirection();
  } else if( hasRecentFace ) {
    TransitionToTurningTowardsFace();
  } else {
    TransitionToFindingFaceInCurrentDirection();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::BehaviorUpdate()
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
    case State::FollowupBehavior:
    {
      if( (_dVars.stateEndTime_s >= 0.0f) && (_dVars.stateEndTime_s < currentTime_s) ) {
        // timeout
        CancelSelf();
      }
    }
      break;
    case State::Invalid:
    case State::DriveOffCharger:
    case State::TurnTowardsPreviousFace:
      // nothing to do, or waiting for action/behavior callback
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::OnBehaviorDeactivated()
{
  if( !_iConfig.exitOnceFound ) {
    _dVars.targetFace.Reset();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToDrivingOffCharger()
{
  SET_STATE(DriveOffCharger);
  auto callback = [this]() {
    const bool hasRecentFace = GetRecentFace( _dVars.targetFace, _dVars.lastFaceTimeStamp_ms );
    if( hasRecentFace ) {
      TransitionToTurningTowardsFace();
    } else {
      TransitionToFindingFaceInCurrentDirection();
    }
  };
  if( _iConfig.driveOffChargerBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.driveOffChargerBehavior.get(), callback );
  } else {
    callback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToLookingInMicDirection()
{
  // not doing anything here, just waiting for a face or timeout
  SET_STATE(LookForFaceInMicDirection);
  _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelFaceLooking_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToTurningTowardsFace()
{
  SET_STATE(TurnTowardsPreviousFace);
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( _dVars.targetFace );
  if( facePtr != nullptr ) {
    auto* action = new TurnTowardsFaceAction( _dVars.targetFace );
    action->SetLockOnClosestFaceAfterTurn( true ); // accept any face once turning in the direction of targetFace
    action->SetRequireFaceConfirmation( true );
    auto callback = [this]( ActionResult result ){
      if( result == ActionResult::SUCCESS ) {
        TransitionToFollowupBehavior();
      } else {
        TransitionToFindingFaceInCurrentDirection();
      }
    };
    CancelDelegates(false);
    DelegateIfInControl(action, callback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToFindingFaceInCurrentDirection()
{
  // not doing anything here, just waiting for a face or timeout
  SET_STATE( FindFaceInCurrentDirection );
  _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelFaceLooking_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToSearchingForFace()
{
  SET_STATE( SearchForFace );
  _dVars.stateEndTime_s = 0;
  if( _iConfig.searchFaceBehavior != nullptr ) {
    _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelSearching_s;
    
    CancelDelegates(false);
    if( _iConfig.searchFaceBehavior->WantsToBeActivated() ) {
      DelegateIfInControl( _iConfig.searchFaceBehavior.get() );
    } else {
      PRINT_NAMED_WARNING("BehaviorFindFaceAndThen.TransitionToSearchingForFace.SearchDoesntWantToActivate",
                          "Search behavior %s doesn't want to activate", _iConfig.searchBehaviorID.c_str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToFollowupBehavior()
{
  if( _iConfig.behaviorOnceFound == nullptr ) {
    // config requests no action
    CancelSelf();
    return;
  }
  
  SET_STATE(FollowupBehavior);
  if( _iConfig.timeUntilCancelFollowup_s >= 0.0f ) {
    _dVars.stateEndTime_s = _iConfig.timeUntilCancelFollowup_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  } else {
    _dVars.stateEndTime_s = -1.0f;
  }

  if( _iConfig.behaviorOnceFoundIsSimpleFace ) {
    std::shared_ptr<ISimpleFaceBehavior> simpleFaceBehavior =
      std::dynamic_pointer_cast<ISimpleFaceBehavior>(_iConfig.behaviorOnceFound);

    if( ANKI_VERIFY( simpleFaceBehavior != nullptr,
                     "BehaviorFindFaceAndThen.TransitionToFollowupBehavior.InvalidCast",
                     "Behavior '%s' (raw ptr %p) not an ISimpleFaceBehavior",
                     _iConfig.behaviorOnceFoundID.c_str(),
                     _iConfig.behaviorOnceFound.get() ) ) {
      simpleFaceBehavior->SetTargetFace( _dVars.targetFace );
    }
  }

  if( _iConfig.behaviorOnceFound->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.behaviorOnceFound.get(), [this]() {
      CancelSelf();
    });
  } else {
    CancelSelf();
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaceAndThen::GetRecentFace( SmartFaceID& faceID, TimeStamp_t& timeStamp_ms )
{
  return GetRecentFaceSince( 0, faceID, timeStamp_ms );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaceAndThen::GetRecentFaceSince( TimeStamp_t sinceTime_ms, SmartFaceID& faceID, TimeStamp_t& timeStamp_ms )
{
  SmartFaceID retFace;
  retFace.Reset();
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  Pose3d facePose;
  TimeStamp_t timeLastFaceObserved = GetBEI().GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = robotInfo.IsPoseInWorldOrigin(facePose);
  if( lastFaceInCurrentOrigin ) {
    timeLastFaceObserved = Anki::Util::Max( sinceTime_ms, timeLastFaceObserved );
    const auto facesObserved = GetBEI().GetFaceWorld().GetFaceIDs(timeLastFaceObserved);
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
void BehaviorFindFaceAndThen::SetState_internal(State state, const std::string& stateName)
{
  _dVars.currentState = state;
  SetDebugStateName(stateName);
}
  

  
}
}
