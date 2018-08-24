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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/iSimpleFaceBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/faceWorld.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Vector {
  
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
const char* const kUseBodyDetectorKey            = "useBodyDetector";
const char* const kAddtlLookTimeIfSawBodyKey     = "additionalLookTimeIfSawBody_s";
const char* const kAddtlSearchTimeIfSawBodyKey   = "additionalSearchTimeIfSawBody_s";
const char* const kUpperPortionLookUpPercent     = "upperPortionLookUpPercent";
const char* const kAnimWhenSeesFaceKey           = "animWhenSeesFace";
const char* const kAnimWhileSearching            = "animWhileSearching";
  
const char* const kDebugName = "BehaviorFindFaceAndThen";

const float kInitialHeadAngle_rad = 0.8f*MAX_HEAD_ANGLE;
  
const TimeStamp_t kTimeToWaitForImage_ms = 2*STATE_MESSAGE_FREQUENCY*ROBOT_TIME_STEP_MS; // = 60ms (*2 in case slowdown)
  
CONSOLE_VAR_RANGED(float, kMinTimeLookInMicDirection_s, "Behaviors.FindFaceAndThen", 0.5f, 0.0f, 2.0f);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaceAndThen::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaceAndThen::DynamicVariables::DynamicVariables()
{
  stateEndTime_s         = 0.0f;
  stateMinTime_s         = 0.0f;
  currentState           = State::Invalid;
  lastFaceTimeStamp_ms   = 0;
  activationTimeStamp_ms = 0;
  timeAtStateChange      = 0;
  sawBodyDuringState     = false;
  timeEndFaceFromBodyBehavior_s = 0.0f;
  targetFace.Reset();
  lastPersonDetected     = Vision::SalientPoint();
  animWhileSearchingTag  = ActionConstants::INVALID_TAG;
  sawBody                = false;
  sawFace                = false;
  sentEvent              = false;
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
  
  _iConfig.useBodyDetector = config.get(kUseBodyDetectorKey, true).asBool();
  if( _iConfig.useBodyDetector ) {
    _iConfig.additionalLookTimeIfSawBody_s = JsonTools::ParseFloat( config, kAddtlLookTimeIfSawBodyKey, GetDebugLabel() );
    _iConfig.additionalSearchTimeIfSawBody_s = JsonTools::ParseFloat( config, kAddtlSearchTimeIfSawBodyKey, GetDebugLabel() );
    _iConfig.upperPortionLookUpPercent = config.get( kUpperPortionLookUpPercent, 0.3f ).asFloat();
    _iConfig.upperPortionLookUpPercent = Util::Clamp( _iConfig.upperPortionLookUpPercent, 0.0f, 1.0f );
  } else {
    _iConfig.additionalLookTimeIfSawBody_s = 0.0f;
    _iConfig.additionalSearchTimeIfSawBody_s = 0.0f;
    _iConfig.upperPortionLookUpPercent = 0.0f;
  }
  
  _iConfig.animWhenSeesFace = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimWhenSeesFaceKey, _iConfig.animWhenSeesFace, GetDebugLabel(), false);
  
  _iConfig.animWhileSearching = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimWhileSearching, _iConfig.animWhileSearching, GetDebugLabel(), false);
  
  ANKI_VERIFY( _iConfig.exitOnceFound == _iConfig.behaviorOnceFoundID.empty(),
               "BehaviorFindFaceAndThen.Ctor.InvalidBehavior",
               "A 'behavior' must be provided, or set 'exitOnceFound' to true" );
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
  if( _iConfig.useBodyDetector ) {
    modifiers.visionModesForActiveScope->insert( {VisionMode::RunningNeuralNet, EVisionUpdateFrequency::Med} );
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
    kUseBodyDetectorKey,
    kAddtlLookTimeIfSawBodyKey,
    kAddtlSearchTimeIfSawBodyKey,
    kUpperPortionLookUpPercent,
    kAnimWhenSeesFaceKey,
    kAnimWhileSearching,
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
  if( _iConfig.behaviorFindFaceInBB != nullptr ) {
    delegates.insert( _iConfig.behaviorFindFaceInBB.get() );
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
  
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(SearchWithinBoundingBox),
                                  BEHAVIOR_CLASS(SearchWithinBoundingBox),
                                  _iConfig.behaviorFindFaceInBB );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  _dVars.activationTimeStamp_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  _dVars.lastFaceTimeStamp_ms = _dVars.activationTimeStamp_ms;
  
  if( _iConfig.animWhileSearching != AnimationTrigger::Count ) {
    // add a parallel search anim action
    auto* animAction = new TriggerLiftSafeAnimationAction{ _iConfig.animWhileSearching, 0 };
    _dVars.animWhileSearchingTag = animAction->GetTag();
    GetBEI().GetRobotInfo().GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
  }
  
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
  
  bool sawBodyWhenLookingForFace = false;
  if( _iConfig.useBodyDetector
      && !_dVars.sawBodyDuringState
      && (  (_dVars.currentState == State::LookForFaceInMicDirection)
         || (_dVars.currentState == State::FindFaceInCurrentDirection)
         || (_dVars.currentState == State::SearchForFace) ) )
  {
    // add a bit more time to when we consider bodies in this direction to make sure the body is actually is in this direction
    const RobotTimeStamp_t time_ms = _dVars.timeAtStateChange + kTimeToWaitForImage_ms;
    sawBodyWhenLookingForFace = GetRecentBodySince( time_ms, _dVars.lastPersonDetected );
    if( sawBodyWhenLookingForFace ) {
      PRINT_CH_INFO( "Behaviors", "BehaviorFindFaceAndThen.BehaviorUpdate.PersonDetected", "Detected a person" );
      _dVars.sawBody = true;
    }
  }
  
  switch( _dVars.currentState ) {
    case State::LookForFaceInMicDirection:
    {
      if( currentTime_s < _dVars.stateEndTime_s ) {
        SmartFaceID newFace;
        RobotTimeStamp_t timeStamp_ms = 0;
        const bool hasFace = GetRecentFaceSince( _dVars.activationTimeStamp_ms, newFace, timeStamp_ms );
        const bool differentOrNew = (newFace != _dVars.targetFace) || (timeStamp_ms > _dVars.lastFaceTimeStamp_ms);
        if( hasFace && differentOrNew && (currentTime_s >= _dVars.stateMinTime_s) ) {
          // a new face was spotted in the direction it's facing, or the same face was spotted in that direction again
          _dVars.targetFace = newFace;
          _dVars.lastFaceTimeStamp_ms = timeStamp_ms;
          TransitionToTurningTowardsFace();
        } else if( sawBodyWhenLookingForFace && !_dVars.sawBodyDuringState ) {
          // a body was seen. extend this state's duration and switch to a behavior that looks for a face
          // attached to a body
          _dVars.sawBodyDuringState = true;
          _dVars.stateEndTime_s += _iConfig.additionalLookTimeIfSawBody_s;
          RunFindFaceFromBodyAction();
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
        RobotTimeStamp_t timeStamp_ms = 0;
        const bool hasFace = GetRecentFace( newFace, timeStamp_ms );
        const bool differentOrNew = (newFace != _dVars.targetFace) || (timeStamp_ms > _dVars.lastFaceTimeStamp_ms);
        if( hasFace && differentOrNew ) {
          // a new face was spotted in the direction it's facing
          _dVars.targetFace = newFace;
          TransitionToTurningTowardsFace();
        } else if( sawBodyWhenLookingForFace && !_dVars.sawBodyDuringState ) {
          // a body was seen. extend this state's duration and switch to a behavior that looks for a face
          // attached to the body
          _dVars.sawBodyDuringState = true;
          if( _dVars.currentState == State::SearchForFace ) {
            _dVars.stateEndTime_s += _iConfig.additionalSearchTimeIfSawBody_s;
            _dVars.timeEndFaceFromBodyBehavior_s = currentTime_s + _iConfig.additionalSearchTimeIfSawBody_s;
          } else if( _dVars.currentState == State::FindFaceInCurrentDirection ) {
            _dVars.stateEndTime_s += _iConfig.additionalLookTimeIfSawBody_s;
          }
          RunFindFaceFromBodyAction();
        } else if( (_dVars.timeEndFaceFromBodyBehavior_s > 0.0f) && (currentTime_s >= _dVars.timeEndFaceFromBodyBehavior_s) ) {
          // the behavior to look for faces attached to bodies should end, so switch back to the search action.
          // in other states, dont do anything
          _dVars.timeEndFaceFromBodyBehavior_s = 0.0f;
          if( _dVars.currentState == State::SearchForFace ) {
            RunSearchFaceBehavior();
          }
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
    case State::LiftingHead:
      // nothing to do, or waiting for action/behavior callback
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::OnBehaviorDeactivated()
{
  StopSearchAnimation(); // does nothing if not running
  
  if( !_iConfig.exitOnceFound ) {
    _dVars.targetFace.Reset();
  }
  // if a parent behavior cancelled us before seeing a face, send info this run
  SendCompletionDASEvent();
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
  SET_STATE(LiftingHead);
  auto* action = new MoveHeadToAngleAction{ kInitialHeadAngle_rad };
  DelegateIfInControl( action, [this](const ActionResult& res){
    // not doing anything here, just waiting for a face or timeout
    SET_STATE(LookForFaceInMicDirection);
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.stateEndTime_s = currTime_s + _iConfig.timeUntilCancelFaceLooking_s;
    _dVars.stateMinTime_s = currTime_s + kMinTimeLookInMicDirection_s;
    _dVars.timeAtStateChange = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    _dVars.sawBodyDuringState = false;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToTurningTowardsFace()
{
  SET_STATE(TurnTowardsPreviousFace);
  _dVars.timeAtStateChange = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  _dVars.timeEndFaceFromBodyBehavior_s = 0.0f;
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( _dVars.targetFace );
  if( facePtr != nullptr ) {
    auto* action = new TurnTowardsFaceAction( _dVars.targetFace );
    action->SetLockOnClosestFaceAfterTurn( true ); // accept any face once turning in the direction of targetFace
    action->SetRequireFaceConfirmation( true );
    if( _iConfig.animWhenSeesFace != AnimationTrigger::Count ) {
      StopSearchAnimation(); // does nothing if not running
      action->SetAnyFaceAnimationTrigger( _iConfig.animWhenSeesFace );
      const u8 tracksToLock = ((u8)AnimTrackFlag::HEAD_TRACK) | ((u8)AnimTrackFlag::LIFT_TRACK) | ((u8)AnimTrackFlag::BODY_TRACK);
      action->SetAnimTracksToLock( tracksToLock );
    }
    auto callback = [this]( ActionResult result ){
      if( result == ActionResult::SUCCESS ) {
        TransitionToFollowupBehavior();
      } else {

        if( IActionRunner::GetActionResultCategory( result ) == ActionResultCategory::ABORT ) {
          // mark this as a "bad" face (reset only when the behavior activates again)
          if( _dVars.targetFace.IsValid() ) {
            PRINT_CH_INFO( "Behaviors", "BehaviorFindFaceAndThen.TurnTowardsFace.Failure.AddFailedFace",
                           "%s: failed to turn towards face '%s' with result %s, skipping this face",
                           GetDebugLabel().c_str(),
                           _dVars.targetFace.GetDebugStr().c_str(),
                           ActionResultToString(result) );

            _dVars.failedFaces.push_back( _dVars.targetFace );
          }
        }

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
  SET_STATE(LiftingHead);
  _dVars.timeEndFaceFromBodyBehavior_s = 0.0f;
  auto* action = new MoveHeadToAngleAction{ kInitialHeadAngle_rad };
  DelegateIfInControl( action, [this](const ActionResult& res){
    // not doing anything here, just waiting for a face or timeout
    SET_STATE( FindFaceInCurrentDirection );
    _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelFaceLooking_s;
    _dVars.stateMinTime_s = 0.0f;
    _dVars.timeAtStateChange = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    _dVars.sawBodyDuringState = false;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToSearchingForFace()
{
  SET_STATE( SearchForFace );
  _dVars.stateEndTime_s = 0;
  _dVars.timeEndFaceFromBodyBehavior_s = 0.0f;
  if( _iConfig.searchFaceBehavior != nullptr ) {
    _dVars.stateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeUntilCancelSearching_s;
    _dVars.stateMinTime_s = 0.0f;
    _dVars.timeAtStateChange = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    _dVars.sawBodyDuringState = false;
    
    RunSearchFaceBehavior();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::TransitionToFollowupBehavior()
{
  StopSearchAnimation(); // does nothing if not running
  
  _dVars.sawFace = true;
  // send the event now instead of deactivation so it occurs before the followup behavior, if there exists one
  SendCompletionDASEvent();
  
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
  _dVars.stateMinTime_s = 0.0f;

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
bool BehaviorFindFaceAndThen::GetRecentFace( SmartFaceID& faceID, RobotTimeStamp_t& timeStamp_ms )
{
  return GetRecentFaceSince( 0, faceID, timeStamp_ms );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaceAndThen::GetRecentFaceSince( RobotTimeStamp_t sinceTime_ms, SmartFaceID& faceID, RobotTimeStamp_t& timeStamp_ms )
{
  SmartFaceID retFace;
  retFace.Reset();
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  Pose3d facePose;
  RobotTimeStamp_t timeLastFaceObserved = GetBEI().GetFaceWorld().GetLastObservedFace(facePose, true);
  const bool lastFaceInCurrentOrigin = robotInfo.IsPoseInWorldOrigin(facePose);
  if( lastFaceInCurrentOrigin ) {
    timeLastFaceObserved = Anki::Util::Max( sinceTime_ms, timeLastFaceObserved );
    const auto facesObserved = GetBEI().GetFaceWorld().GetFaceIDs(timeLastFaceObserved);
    for( const auto& faceID : facesObserved ) {
      bool faceOK = true;
      for( const auto& smartFace : _dVars.failedFaces ) {
        if( smartFace.MatchesFaceID(faceID) ) {
          // skip this face
          faceOK = false;
          break;
        }
      }
      if( faceOK ) {
        retFace = GetBEI().GetFaceWorld().GetSmartFaceID( faceID );
        break;
      }
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
     
bool BehaviorFindFaceAndThen::GetRecentBodySince( RobotTimeStamp_t sinceTime_ms, Vision::SalientPoint& person ) const
{
  if( sinceTime_ms > GetBEI().GetRobotInfo().GetLastImageTimeStamp() ) {
    return false;
  }
  
  const auto& salientPtsComponent = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> bodiesFound;
  salientPtsComponent.GetSalientPointSinceTime( bodiesFound,
                                                Vision::SalientPointType::Person,
                                                sinceTime_ms );
 
  if( bodiesFound.empty() ) {
    return false;
  }
 
  // choose the most recent body, or the largest (big boned) if tied
  auto maxElementIter = std::max_element( bodiesFound.begin(),
                                          bodiesFound.end(),
                                          [](const Vision::SalientPoint& p1, const Vision::SalientPoint& p2) {
                                            if( p1.timestamp == p2.timestamp ) {
                                              return p1.area_fraction < p2.area_fraction;
                                            } else {
                                              return p1.timestamp < p2.timestamp;
                                            }
                                          } );
  person = *maxElementIter;
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::RunSearchFaceBehavior()
{
  CancelDelegates( false );
  if( _iConfig.searchFaceBehavior->WantsToBeActivated() ) {
    PRINT_CH_INFO( "Behaviors", "BehaviorFindFaceAndThen.RunSearchFaceBehavior.Run", "Running search face behavior" );
    DelegateIfInControl( _iConfig.searchFaceBehavior.get() );
  } else {
    PRINT_NAMED_WARNING( "BehaviorFindFaceAndThen.RunSearchFaceBehavior.SearchDoesntWantToActivate",
                         "Search behavior %s doesn't want to activate", _iConfig.searchBehaviorID.c_str() );
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::RunFindFaceFromBodyAction()
{
  // pass the bounding box values
  // find the extents of the bounding box
  float maxX = 0.f;
  float minX = 1.f;
  float maxY = 0.f;
  float minY = 1.f;
  for( const auto& shape : _dVars.lastPersonDetected.shape ) {
    if( shape.x > maxX ) {
      maxX = shape.x;
    }
    if( shape.x < minX ) {
      minX = shape.x;
    }
    if( shape.y > maxY ) {
      maxY = shape.y;
    }
    if( shape.y < minY ) {
      minY = shape.y;
    }
  }
  
  if( _iConfig.upperPortionLookUpPercent != 1.0 ) {
    const float length = maxY - minY;
    const float reducedLength = _iConfig.upperPortionLookUpPercent * length;
    minY = maxY - reducedLength;
  }
  
  _iConfig.behaviorFindFaceInBB->SetNewBoundingBox( minX, maxX, minY, maxY );
  if( _iConfig.behaviorFindFaceInBB->WantsToBeActivated() ) {
    PRINT_CH_INFO( "Behaviors",
                   "BehaviorFindFaceAndThen.RunFindFaceFromBodyAction.Run",
                   "Running find-face-from-body behavior" );
    CancelDelegates(false); // it's possible the regular find faces behavior is running
    DelegateIfInControl( _iConfig.behaviorFindFaceInBB.get(), [this]() {
      // not expected to end, but just in case it does, resume with the other search behavior
      PRINT_CH_INFO( "Behaviors", "BehaviorFindFaceAndThen.RunFindFaceFromBodyAction.UnexpectedEnd",
                     "Behavior SearchWithinBoundingBox ended unexpectedly. Recovering" );
      if( _dVars.currentState == State::SearchForFace ) {
        RunSearchFaceBehavior();
      }
    });
  } else {
    PRINT_NAMED_WARNING( "BehaviorFindFaceAndThen.RunFindFaceFromBodyAction.FindFromBodyDoesntWantToActivate",
                         "Find face from body behavior doesn't want to activate" );
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::SetState_internal(State state, const std::string& stateName)
{
  _dVars.currentState = state;
  SetDebugStateName(stateName);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::SendCompletionDASEvent()
{
  if( _dVars.sentEvent ) {
    // only send one event
    return;
  }
  DASMSG(behavior_findfaceduration, "behavior.findfaceduration", "Info about how long it took a voice command to find a face");
  DASMSG_SET(i1, (int) Util::SecToMilliSec(GetActivatedDuration()), "Time spent looking (ms)");
  DASMSG_SET(i2, (int)_dVars.sawFace, "Whether a face was seen / voice command successful [may not be accurate for takeaphoto]" );
  DASMSG_SET(i3, (int)_dVars.sawBody, "Whether a body was detected when looking for a face" );
  DASMSG_SEND();
  
  _dVars.sentEvent = true;
}
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaceAndThen::StopSearchAnimation()
{
  if( _dVars.animWhileSearchingTag != ActionConstants::INVALID_TAG ) {
    GetBEI().GetRobotInfo().GetActionList().Cancel( _dVars.animWhileSearchingTag );
    _dVars.animWhileSearchingTag = ActionConstants::INVALID_TAG;
  }
}
  
}
}
