/**
 * File: behaviorEnrollFace.cpp
 *
 * Author: Andrew Stein
 * Date:   11/22/2016
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 **/


 // TODO:  VIC-26 - Migrate Audio in BehaviorEnrollFace
// This class controlls music state which is not realevent when playing audio on the robot
#include "engine/aiComponent/behaviorComponent/behaviors/meetCozmo/behaviorEnrollFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/actions/sayTextAction.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/utils/timer.h"

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/trackedFace.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/enrolledFaceStorage.h"

#include "util/console/consoleInterface.h"

#define CONSOLE_GROUP "Behavior.EnrollFace"

namespace Anki {
namespace Cozmo {

namespace {

CONSOLE_VAR(TimeStamp_t,       kEnrollFace_TimeoutForReLookForFace_ms,          CONSOLE_GROUP, 1500);

// Thresholds for when to update face ID based on pose
CONSOLE_VAR(f32,               kEnrollFace_UpdateFacePositionThreshold_mm,      CONSOLE_GROUP, 100.f);
CONSOLE_VAR(f32,               kEnrollFace_UpdateFaceAngleThreshold_deg,        CONSOLE_GROUP, 45.f);

// Default timeout for overall enrollment (e.g. to be looking for a face or waiting for enrollment to complete)
CONSOLE_VAR(f32,               kEnrollFace_Timeout_sec,                         CONSOLE_GROUP, 15.f);
CONSOLE_VAR(f32,               kEnrollFace_TimeoutMax_sec,                      CONSOLE_GROUP, 35.f);

// Amount of "extra" time to add each time we re-start actually enrolling, in case we lose the face
// mid way or take a while to initially find the face, up to the max timeout
CONSOLE_VAR(f32,               kEnrollFace_TimeoutExtraTime_sec,                CONSOLE_GROUP, 8.f);

// Amount to drive forward once face is found to signify intent
CONSOLE_VAR(f32,               kEnrollFace_DriveForwardIntentDist_mm,           CONSOLE_GROUP, 14.f);
CONSOLE_VAR(f32,               kEnrollFace_DriveForwardIntentSpeed_mmps,        CONSOLE_GROUP, 75.f);

// Minimum angles to turn during tracking to keep the robot moving and looking alive
CONSOLE_VAR(f32,               kEnrollFace_MinTrackingPanAngle_deg,             CONSOLE_GROUP, 4.0f);
CONSOLE_VAR(f32,               kEnrollFace_MinTrackingTiltAngle_deg,            CONSOLE_GROUP, 4.0f);

// Min/max distance to backup while looking for a face, up to max total amount
CONSOLE_VAR(f32,               kEnrollFace_MinBackup_mm,                        CONSOLE_GROUP,  5.f);
CONSOLE_VAR(f32,               kEnrollFace_MaxBackup_mm,                        CONSOLE_GROUP, 15.f);
CONSOLE_VAR(f32,               kEnrollFace_MaxTotalBackup_mm,                   CONSOLE_GROUP, 50.f);

CONSOLE_VAR(s32,               kEnrollFace_NumImagesToWait,                     CONSOLE_GROUP, 3);

// Number of faces to consider "too many" and forced timeout when seeing that many
CONSOLE_VAR(s32,               kEnrollFace_DefaultMaxFacesVisible,              CONSOLE_GROUP, 1); // > this is "too many"
CONSOLE_VAR(f32,               kEnrollFace_DefaultTooManyFacesTimeout_sec,      CONSOLE_GROUP, 2.f);
CONSOLE_VAR(f32,               kEnrollFace_DefaultTooManyFacesRecentTime_sec,   CONSOLE_GROUP, 0.5f);

static const char * const kLogChannelName = "FaceRecognizer";
static const char * const kMaxFacesVisibleKey = "maxFacesVisible";
static const char * const kTooManyFacesTimeoutKey = "tooManyFacesTimeout_sec";
static const char * const kTooManyFacesRecentTimeKey = "tooManyFacesRecentTime_sec";
  
}
  
// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _state = State::s; DEBUG_SET_STATE(s); } while(0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::BehaviorEnrollFace(const Json::Value& config)
: ICozmoBehavior(config)
, _settings(new ExternalInterface::SetFaceToEnroll())
{
  
  SubscribeToTags({{
    EngineToGameTag::UnexpectedMovement,
    EngineToGameTag::RobotChangedObservedFaceID,
    EngineToGameTag::RobotOffTreadsStateChanged,
    EngineToGameTag::CliffEvent
  }});
  
  SubscribeToTags({{
    GameToEngineTag::SetFaceToEnroll,
    GameToEngineTag::CancelFaceEnrollment,
  }});
  
  // If Cozmo sees more than maxFacesVisible for longer than tooManyFacesTimeout seconds while looking for a face or
  // enrolling a face, then the behavior transitions to the TimedOut state and then returns the SawMultipleFaces
  // FaceEnrollmentResult.
  _maxFacesVisible = config.get(kMaxFacesVisibleKey, kEnrollFace_DefaultMaxFacesVisible).asInt();
  _tooManyFacesTimeout_sec = config.get(kTooManyFacesTimeoutKey, kEnrollFace_DefaultTooManyFacesTimeout_sec).asFloat();
  _tooManyFacesRecentTime_sec = config.get(kTooManyFacesRecentTimeKey, kEnrollFace_DefaultTooManyFacesRecentTime_sec).asFloat();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::CheckForIntentData() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntent* intent = uic.TakePreservedUserIntentOwnership( USER_INTENT(meet_victor) );
  if( intent != nullptr ) {
    const auto& meetVictor = intent->Get_meet_victor();
    _settings->name = meetVictor.username;
    _settings->observedID = Vision::UnknownFaceID;
    _settings->saveID = 0;
    _settings->saveToRobot = true;
    _settings->sayName = true;
    _settings->useMusic = false;
    delete intent;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::WantsToBeActivatedBehavior() const
{
  CheckForIntentData();
  // This behavior is activatable iff a face enrollment has been requested
  return IsEnrollmentRequested();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEnrollFace::InitEnrollmentSettings()
{
  if( !IsEnrollmentRequested() ) {
    // this can happen in tests
    PRINT_NAMED_WARNING("BehaviorEnrollFace.InitEnrollmentSettings.FaceEnrollmentNotRequested",
                        "BehaviorEnrollFace started without an enrollment request");
  }
  
  _faceID        = _settings->observedID;
  _saveID        = _settings->saveID;
  _faceName      = _settings->name;
  _saveToRobot   = _settings->saveToRobot;
  _useMusic      = _settings->useMusic;
  _sayName       = _settings->sayName;
  
  if(_faceName.empty())
  {
    PRINT_NAMED_ERROR("BehaviorEnrollFace.InitEnrollmentSettings.EmptyName", "");
    return RESULT_FAIL;
  }
  
  if( GetBEI().GetVisionComponent().IsNameTaken( _faceName ) ) {
    TransitionToSayingIKnowThatName();
    return RESULT_FAIL;
  }
  
  // If saveID is specified and we've already seen it (so it's in FaceWorld), make
  // sure that it is the ID of a _named_ face
  if(_saveID != Vision::UnknownFaceID)
  {
    const Face* face = GetBEI().GetFaceWorld().GetFace(_saveID);
    if(nullptr != face && !face->HasName())
    {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.InitEnrollmentSettings.UnnamedSaveID",
                          "Face with SaveID:%d has no name", _saveID);
      return RESULT_FAIL;
    }
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::OnBehaviorActivated()
{
  // Check if we were interrupted and need to fast forward:
  switch(_state)
  {
    case State::SayingName:
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal.FastForwardToSayingName", "");
      TransitionToSayingName();
      return;
    }
      
    case State::SayingIKnowThatName:
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal.FastForwardToSayingIKnowThatName", "");
      TransitionToSayingIKnowThatName();
      return;
    }

    case State::SavingToRobot:
    case State::SaveFailed:
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal.FastForwardToSavingToRobot", "");
      TransitionToSavingToRobot();
      return;
    }
      
    case State::ScanningInterrupted:
    {
      // If we were interrupted while getting out of the scanning animation and have
      // now resumed, we need to complete the animation
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal.FastForwardToScanningInterrupted", "");
      TransitionToScanningInterrupted();
      return;
    }
    default:
      // Not fast forwarding: just start at the beginning
      SET_STATE(NotStarted);
  }
  
  const Result settingsResult = InitEnrollmentSettings();
  if(RESULT_OK != settingsResult)
  {
    PRINT_NAMED_WARNING("BehaviorEnrollFace.InitInternal.BadSettings",
                        "Disabling enrollment");
    DisableEnrollment();
    return;
  }
  
  {
    // send a status update to the app that MeetVictor has started with _faceName.
    // todo: this will be superceded by some sort of robot status VIC-1423
    if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
      ExternalInterface::MeetVictorStarted status( _faceName );
      GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(status)));
    }
  }
  
  // Settings ok: initialize rest of behavior state
  _saveSucceeded = false;
  
  _observedUnusableID  = Vision::UnknownFaceID;
  _observedUnusableName.clear();
  
  _enrollingSpecificID = (_faceID != Vision::UnknownFaceID);
  
  _startTime_sec       = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeout_sec         = kEnrollFace_Timeout_sec; // initial timeout, can change as we run
  
  _lastFaceSeenTime_ms            = 0;
  _startedSeeingMultipleFaces_sec = 0.f;
  
  _lastRelBodyAngle    = 0.f;
  _totalBackup_mm      = 0.f;
  
  _saveEnrollResult    = ActionResult::NOT_STARTED;
  _saveAlbumResult     = ActionResult::NOT_STARTED;
  
  // Reset flag in FaceWorld because we're starting a new enrollment and will
  // be waiting for this new enrollment to be "complete" after this
  GetBEI().GetFaceWorldMutable().SetFaceEnrollmentComplete(false);
  
  // Make sure enrollment is enabled for session-only faces when we start. Otherwise,
  // we won't even be able to start enrollment because everything will remain a
  // "tracking only" face.
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);
  
  PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal",
                "Initialize with ID=%d and name '%s', to be saved to ID=%d",
                _faceID, Util::HidePersonallyIdentifiableInfo(_faceName.c_str()), _saveID);
    
  // Start with this timeout (may increase as the behavior runs)
  _timeout_sec = kEnrollFace_Timeout_sec;
  
  // First thing we want to do is turn towards the face and make sure we see it
  TransitionToLookingForFace();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  // See if we were in the midst of finding or enrolling a face but the enrollment is
  // no longer requested, then we've been cancelled
  if((State::LookingForFace == _state || State::Enrolling == _state) && !IsEnrollmentRequested())
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateInternal_Legacy.EnrollmentCancelled", "In state: %s",
                  _state == State::LookingForFace ? "LookingForFace" : "Enrolling");
    CancelSelf();
    return;
  }
  
  switch(_state)
  {
    case State::Success:
    case State::NotStarted:
    case State::TimedOut:
    case State::Failed_WrongFace:
    case State::Failed_UnknownReason:
    case State::Failed_NameInUse:
    case State::SaveFailed:
    case State::Cancelled:
    {
      CancelSelf();
      return;
    }
      
    case State::LookingForFace:
    {
      // Check to see if the face we've been enrolling has changed based on what was
      // observed since the last tick
      UpdateFaceToEnroll();
      break;
    }
      
    case State::SayingName:
    case State::SayingIKnowThatName:
    case State::SavingToRobot:
    case State::EmotingConfusion:
    case State::ScanningInterrupted:
    {
      // Nothing specific to do: just waiting for animation/save to complete
      break;
    }
      
    case State::Enrolling:
    {
      // Check to see if we're done
      if(GetBEI().GetFaceWorld().IsFaceEnrollmentComplete())
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CheckIfDone.ReachedEnrollmentCount", "");
        
        // tell the app we've finished scanning
        // todo: replace with generic status VIC-1423
        if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
          ExternalInterface::MeetVictorFaceScanComplete status;
          GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(status)));
        }
        
        // If we complete successfully, unset the observed ID/name
        _observedUnusableID = Vision::UnknownFaceID;
        _observedUnusableName.clear();
        GetBEI().GetVisionComponent().AssignNameToFace(_faceID, _faceName, _saveID);

        // Note that we will wait to disable face enrollment until the very end of
        // the behavior so that we remain resume-able from reactions, in case we
        // are interrupted after this point (e.g. while playing the sayname animations).
        
        TransitionToSayingName();
      }
      else if(HasTimedOut())
      {
        // Need to play scanning get-out because we timed out while enrolling
        TransitionToScanningInterrupted();
      }
      else
      {
        // Check to see if the face we've been enrolling has changed based on what was
        // observed since the last tick
        UpdateFaceToEnroll();

        const auto& robotInfo = GetBEI().GetRobotInfo();
        // If we haven't seen the person (and only the one person) in too long, go back to looking for them
        if(robotInfo.GetLastImageTimeStamp() - _lastFaceSeenTime_ms > kEnrollFace_TimeoutForReLookForFace_ms)
        {
          _lastFaceSeenTime_ms = 0;
          
          // Go back to looking for face
          PRINT_NAMED_INFO("BehaviorEnrollFace.CheckIfDone.NoLongerSeeingFace",
                           "Have not seen face %d in %dms, going back to LookForFace state for %s",
                           _faceID, kEnrollFace_TimeoutForReLookForFace_ms,
                           _enrollingSpecificID ? ("face " + std::to_string(_faceID)).c_str() : "any face");
          
          // If we are not enrolling a specific face ID, we are allowed to try again
          // with a new face, so don't hang waiting to see the one we previously picked
          if(!_enrollingSpecificID)
          {
            _faceID = Vision::UnknownFaceID;
          }
          
          TransitionToLookingForFace();
        }
      }
      
      break;
    } // case State::Enrolling
      
  } // switch(_state) 
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::OnBehaviorDeactivated()
{
  // Leave general-purpose / session-only enrollment enabled (i.e. not for a specific face)
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);

  const auto& robotInfo = GetBEI().GetRobotInfo();
  // if on the charger, we're exiting to the on charger reaction, unity is going to try to cancel but too late.
  if( robotInfo.IsOnChargerContacts() )
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.CancelBecauseOnCharger","");
    SET_STATE(Cancelled);
  }
  
  ExternalInterface::FaceEnrollmentCompleted info;
  
  if( _state == State::EmotingConfusion ) {
    // interrupted while in a transient animation state. Replace with the reason for being
    // in this state
    _state = _failedState;
  }
  
  const bool wasSeeingMultipleFaces = _startedSeeingMultipleFaces_sec > 0.f;
  const bool observedUnusableFace = _observedUnusableID != Vision::UnknownFaceID && !_observedUnusableName.empty();
  
  // If observed ID/face are set, then it means we never found a valid, unnamed face to use
  // for enrollment, so return those in the completion message and indicate this in the result.
  // NOTE: Seeing multiple faces effectively takes precedence here.
  if(_state == State::Failed_WrongFace || (_state == State::TimedOut &&
                                           !wasSeeingMultipleFaces &&
                                           observedUnusableFace))
  {
    info.faceID = _observedUnusableID;
    info.name   = _observedUnusableName;
    info.result = FaceEnrollmentResult::SawWrongFace;
  }
  else
  {
    if(_saveID != Vision::UnknownFaceID)
    {
      // We just merged the enrolled ID (faceID) into saveID, so report saveID as
      // "who" was enrolled
      info.faceID = _saveID;
    }
    else
    {
      info.faceID = _faceID;
    }
    
    info.name   = _faceName;
  
    switch(_state)
    {
      case State::TimedOut:
      {
        if(wasSeeingMultipleFaces)
        {
          info.result = FaceEnrollmentResult::SawMultipleFaces;
        }
        else
        {
          info.result = FaceEnrollmentResult::TimedOut;
        }
        break;
      }

      case State::Cancelled:
        info.result = FaceEnrollmentResult::Cancelled;
        break;
        
      case State::LookingForFace:
      case State::Enrolling:
      case State::ScanningInterrupted:
      case State::SayingName:
      case State::SayingIKnowThatName:
      case State::SavingToRobot:
      case State::SaveFailed:
        // If we're stopping in any of these states without having timed out
        // then something else is keeping us from completing and the assumption
        // is that we'll resume and finish shortly
        info.result = FaceEnrollmentResult::Incomplete;
        break;
        
      case State::Success:
        info.result = FaceEnrollmentResult::Success;
        break;
      
      case State::Failed_NameInUse:
        info.result = FaceEnrollmentResult::NameInUse;
        break;
        
      case State::NotStarted:
      case State::Failed_UnknownReason:
        info.result = FaceEnrollmentResult::UnknownFailure;
        break;
      
      case State::Failed_WrongFace:
      case State::EmotingConfusion:
        // Should have been handled above
        PRINT_NAMED_ERROR("BehaviorEnrollFace.StopInternal.UnexpectedState",
                          "Failed_WrongFace state not expected here");
        break;
        
    } // switch(_state)
  }
  
  // If incomplete, we are being interrupted by something. Don't broadcast completion
  // and don't disable face enrollment.
  if(info.result != FaceEnrollmentResult::Incomplete)
  {
    DisableEnrollment();
    
    // If enrollment did not succeed (but is complete) and we're enrolling a *new* face:
    // It is possible that the vision system (on its own thread!) actually finished enrolling internally. Therefore we
    // want to erase any *new* face (not a face that was being re-enrolled) since it will not be communicated out in the
    // enrollment result as successfully enrolled, and thus would mean the engine's known faces would be out of sync
    // with the external world. This is largely precautionary.
    const bool isNewEnrollment = Vision::UnknownFaceID != _faceID && Vision::UnknownFaceID == _saveID;
    if(info.result != FaceEnrollmentResult::Success && isNewEnrollment)
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.ErasingNewlyEnrolledFace",
                    "Erasing new face %d as a precaution because we are about to report failure result: %s",
                    _faceID, EnumToString(info.result));
      GetBEI().GetVisionComponent().EraseFace(_faceID);
    }
    
    if(info.result == FaceEnrollmentResult::Success)
    {
      BehaviorObjectiveAchieved(BehaviorObjective::EnrolledFaceWithName);
    }
    
    // Log enrollment to DAS, with result type
    Util::sEventF("robot.face_enrollment", {{DDATA, EnumToString(info.result)}}, "%d", info.faceID);
    
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.BroadcastCompletion",
                  "In state:%hhu, FaceEnrollmentResult=%s",
                  _state, EnumToString(info.result));
    
    if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
      GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(info)));
    }
    
    // Done (whether success or failure), so reset state for next run
    SET_STATE(NotStarted);
  }
  
  PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.StopInternal.FinalState",
                 "Stopping EnrollFace in state %s", GetDebugStateName().c_str());
}

#pragma mark -
#pragma mark State Machine

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsEnrollmentRequested() const
{
  const bool hasName = !_settings->name.empty();
  return hasName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::DisableEnrollment()
{
  _settings->name.clear();
  // Leave "session-only" face enrollment enabled when we finish
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::HasTimedOut() const
{
  const f32 currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasTimedOut = (currTime_sec > _startTime_sec + _timeout_sec);
  const bool hasSeenTooManyFacesTooLong = (_startedSeeingMultipleFaces_sec > 0.f &&
                                           (currTime_sec > _startedSeeingMultipleFaces_sec + _tooManyFacesTimeout_sec));
  
  if(hasTimedOut)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.BehaviorTimedOut",
                  "TimedOut after %.1fsec in State:%s",
                  _timeout_sec, GetDebugStateName().c_str());
  }
  
  if(hasSeenTooManyFacesTooLong)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.TooManyFacesTooLong",
                  "Saw > %d faces for longer than %.1fsec in State:%s",
                  _maxFacesVisible, _tooManyFacesTimeout_sec, GetDebugStateName().c_str());
  }
  
  return hasTimedOut || hasSeenTooManyFacesTooLong;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::CanMoveTreads() const
{
  if(GetBEI().GetOffTreadsState() != OffTreadsState::OnTreads)
  {
    return false;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if(robotInfo.GetCliffSensorComponent().IsCliffDetected())
  {
    return false;
  }
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToLookingForFace()
{
  const bool playScanningGetOut = (State::Enrolling == _state);
  
  SET_STATE(LookingForFace);

  IActionRunner* action = new CompoundActionSequential({
    CreateTurnTowardsFaceAction(_faceID, _saveID, playScanningGetOut),
    new WaitForImagesAction(kEnrollFace_NumImagesToWait, VisionMode::DetectingFaces),
  });

  
  // Make sure we stop tracking/scanning if necessary (CreateTurnTowardsFaceAction can create
  // a tracking action)
  CancelDelegates(false);
  
  DelegateIfInControl(action, [this]()
              {
                if(_lastFaceSeenTime_ms == 0)
                {
                  // Still no face seen: either time out or try again
                  if(HasTimedOut())
                  {
                    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.LookingForFace.TimedOut", "");
                    TransitionToFailedState(State::TimedOut,"TimedOut");
                  }
                  else
                  {
                    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.LookingForFace.NoFaceSeen",
                                  "Trying again. FaceID:%d",
                                  _faceID);
                    
                    DelegateIfInControl(CreateLookAroundAction(),
                                        &BehaviorEnrollFace::TransitionToLookingForFace);
                  }
                }
                else
                {
                  // We've seen a face, so time to start enrolling it
                  
                  // Give ourselves a little more time to finish now that we've seen a face, but
                  // don't go over the max timeout
                  _timeout_sec = std::min(kEnrollFace_TimeoutMax_sec, _timeout_sec + kEnrollFace_TimeoutExtraTime_sec);
                  
                  PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.LookingForFace.FaceSeen",
                                "Found face %d to enroll. Timeout set to %.1fsec",
                                _faceID, _timeout_sec);

                  auto getInAnimAction = new TriggerAnimationAction(AnimationTrigger::MeetCozmoLookFaceGetIn);
                  
                  IActionRunner* action = nullptr;
                  if(CanMoveTreads())
                  {
                    SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(_faceID);
                    // Turn towards the person we've chosen to enroll, play the "get in" animation
                    // to start "scanning" and move towards the person a bit to show intentionality
                    action = new CompoundActionSequential({
                      new TurnTowardsFaceAction(smartID, M_PI, false),
                      new CompoundActionParallel({
                        getInAnimAction,
                        new DriveStraightAction(kEnrollFace_DriveForwardIntentDist_mm,
                                                kEnrollFace_DriveForwardIntentSpeed_mmps, false)
                      })
                    });
                  }
                  else
                  {
                    // Just play the get-in if we aren't able to move the treads
                    action = getInAnimAction;
                  }
                  
                  // tell the app we're beginning enrollment
                  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
                    // todo: replace with generic status VIC-1423
                    ExternalInterface::MeetVictorFaceScanStarted status;
                    GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(status)));
                  }
                  
                  DelegateIfInControl(action, &BehaviorEnrollFace::TransitionToEnrolling);
                }
              });
  
}
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToEnrolling()
{
  SET_STATE(Enrolling);
  
  // Actually enable directed enrollment of the selected face in the vision system
  GetBEI().GetFaceWorldMutable().Enroll(_faceID);

  TrackFaceAction* trackAction = new TrackFaceAction(_faceID);
    
  if(!CanMoveTreads())
  {
    // Only move head during tracking if robot is off the ground or a cliff is detected
    // (the latter can happen if picked up but held very level)
    trackAction->SetMode(ITrackAction::Mode::HeadOnly);
  }
  
  // Add constant small movement
  // TODO: Make this a configuration parameter
  trackAction->SetTiltTolerance(DEG_TO_RAD(kEnrollFace_MinTrackingTiltAngle_deg));
  trackAction->SetPanTolerance(DEG_TO_RAD(kEnrollFace_MinTrackingPanAngle_deg));
  trackAction->SetClampSmallAnglesToTolerances(true);
  
  // Play the scanning animation in parallel while we're tracking
  const s32 numLoops = 0; // loop forever
  TriggerAnimationAction* scanLoop  = new TriggerAnimationAction(AnimationTrigger::MeetCozmoScanningIdle, numLoops);
  
  CompoundActionParallel* compoundAction = new CompoundActionParallel({trackAction, scanLoop});
  
  // Tracking never completes. UpdateInternal will watch for timeout or for
  // face enrollment to complete and stop this behavior or transition to
  // a completion state.
  DelegateIfInControl(compoundAction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToScanningInterrupted()
{
  SET_STATE(ScanningInterrupted);
  
  // Make sure we stop tracking necessary (in case we timed out while tracking)
  CancelDelegates(false);

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::MeetCozmoLookFaceInterrupt),
              [this]() {
                SET_STATE(TimedOut);
              });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToSayingName()
{
  SET_STATE(SayingName);
  
  // Stop tracking/scanning the face
  CancelDelegates(false);
  
  // Get out of the scanning face
  CompoundActionSequential* finalAnimation = new CompoundActionSequential({
    new TriggerAnimationAction(AnimationTrigger::MeetCozmoLookFaceGetOut)
  });
  
  if(_sayName)
  {
    if(_saveID == Vision::UnknownFaceID)
    {
      // If we're not being told which ID to save to, then assume this is a
      // first-time enrollment and play the bigger sequence of animations,
      // along with special music state
      // TODO: PostMusicState should take in a GameState::Music, making the cast unnecessary...
      if(_useMusic)
      {
        // NOTE: it will be up to the caller to stop this music
//        robot.GetRobotAudioClient()->PostMusicState((AudioMetaData::GameState::GenericState)AudioMetaData::GameState::Music::Minigame__Meet_Cozmo_Say_Name, false, 0);
      }
      
      {
        // 1. Say name once
        SayTextAction* sayNameAction1 = new SayTextAction(_faceName, SayTextIntent::Name_FirstIntroduction_1);
        sayNameAction1->SetAnimationTrigger(AnimationTrigger::MeetCozmoFirstEnrollmentSayName);
        finalAnimation->AddAction(sayNameAction1);
      }
      
      {
        // 2. Repeat name
        SayTextAction* sayNameAction2 = new SayTextAction(_faceName, SayTextIntent::Name_FirstIntroduction_2);
        sayNameAction2->SetAnimationTrigger(AnimationTrigger::MeetCozmoFirstEnrollmentRepeatName);
        finalAnimation->AddAction(sayNameAction2);
      }
      
      {
        // 3. Big celebrate (no name being said)
        TriggerAnimationAction* celebrateAction = new TriggerAnimationAction(AnimationTrigger::MeetCozmoFirstEnrollmentCelebration);
        finalAnimation->AddAction(celebrateAction);
      }
    }
    else
    {
      // This is a re-enrollment, so do the more subdued animation
      SayTextAction* sayNameAction = new SayTextAction(_faceName, SayTextIntent::Name_Normal);
      sayNameAction->SetAnimationTrigger(AnimationTrigger::MeetCozmoReEnrollmentSayName);
      
      finalAnimation->AddAction(sayNameAction);
    }
    
    // This is kinda hacky, but we could have used up a lot of our timeout time
    // during enrollment and don't want to cutoff the final animation action (which could be
    // pretty long if it's a first time enrollment), so increase our timeout at this point.
    _timeout_sec += 30.f;
  }
  
  // Note: even if the animation fails for some reason, we will still continue with the behavior
  DelegateIfInControl(finalAnimation, [this](ActionResult result)
  {
    if(ActionResult::SUCCESS != result)
    {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToSayingName.FinalAnimationFailed", "");
    }
    else
    {
      if(_saveToRobot)
      {
        TransitionToSavingToRobot();
      }
      else
      {
        SET_STATE(Success);
      }
    }
  });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToSayingIKnowThatName()
{
  SET_STATE(SayingIKnowThatName);
  
  CancelDelegates(false);
  
  if(_sayName) {
    
    
    // todo: locale
    const std::string sentence = "eye already know a " + _faceName;
    SayTextAction* speakAction = new SayTextAction(sentence, SayTextIntent::Text);
    speakAction->SetAnimationTrigger(AnimationTrigger::MeetCozmoDuplicateName);
    
  
    DelegateIfInControl(speakAction, [this](ActionResult result) {
      SET_STATE(Failed_NameInUse);
    });
  } else {
    TransitionToFailedState(State::Failed_NameInUse,"Failed_NameInUse");
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToFailedState( State state, const std::string& stateName )
{
  SET_STATE(EmotingConfusion);
  _failedState = state;
  
  CancelDelegates(false);
  
  auto* action = new TriggerLiftSafeAnimationAction(AnimationTrigger::MeetCozmoConfusion);
  
  DelegateIfInControl(action, [this, state, &stateName](ActionResult result) {
    if( ActionResult::SUCCESS != result ) {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToFailedState.FinalAnimationFailed", "");
    }
    _state = state;
    SetDebugStateName(stateName);
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToSavingToRobot()
{
  SET_STATE(SavingToRobot);
  
  // Trigger a save. These callbacks set flags on save completion. The waitForSave
  // lambda/action looks for these flags to be set.
  std::function<void(NVStorage::NVResult)> saveEnrollCallback = [this](NVStorage::NVResult result)
  {
    if(result == NVStorage::NVResult::NV_OKAY) {
      _saveEnrollResult = ActionResult::SUCCESS;
    } else {
      _saveEnrollResult = ActionResult::ABORT;
    }
  };
  
  std::function<void(NVStorage::NVResult)> saveAlbumCallback = [this](NVStorage::NVResult result)
  {
    if(result == NVStorage::NVResult::NV_OKAY) {
      _saveAlbumResult = ActionResult::SUCCESS;
    } else {
      _saveAlbumResult = ActionResult::ABORT;
    }
  };
  
  GetBEI().GetVisionComponent().SaveFaceAlbumToRobot(saveAlbumCallback, saveEnrollCallback);
  
  std::function<bool(Robot& robot)> waitForSave = [this](Robot& robot) -> bool
  {
    // Wait for Save to complete (success or failure)
    if(ActionResult::NOT_STARTED != _saveEnrollResult &&
       ActionResult::NOT_STARTED != _saveAlbumResult)
    {
      if(ActionResult::SUCCESS == _saveEnrollResult &&
         ActionResult::SUCCESS == _saveAlbumResult)
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.TransitionToSavingToRobot.NVStorageSaveSucceeded", "");
        return true;
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToSavingToRobot.NVStorageSaveFailed",
                            "SaveEnroll:%s SaveAlbum:%s. Erasing face.",
                            EnumToString(_saveEnrollResult),
                            EnumToString(_saveAlbumResult));
        
        if(Vision::UnknownFaceID == _saveID)
        {
          // if this was a new enrollment, then this person is not going to be
          // known when we reconnect to the robot, so report failure so that we
          // erase them from memory when the behavior stops. If this was a re-enrollment,
          // then we'll just silently fail to remember this particular enrollment data,
          // since that won't have a big effect on the user.
          return false;
        }
        else
        {
          // Failure to save doesn't really matter if this was a re-enrollment. The
          // only downside is that this most recent enrollment won't get reloaded
          // from the robot next time we restart, but that's not a huge loss and
          // it will still get used for the remainder of this session.
          return true;
        }
      }
    }
    return false;
  };
  
  const f32 kMaxSaveTime_sec = 5.f; // Don't wait the default (long) time for save to complete
  WaitForLambdaAction* action = new WaitForLambdaAction(waitForSave, kMaxSaveTime_sec);
  
  DelegateIfInControl(action, [this](ActionResult actionResult) {
    if (ActionResult::SUCCESS == actionResult) {
      SET_STATE(Success);
    } else {
      SET_STATE(SaveFailed);
    }
  });
  
}
  
#pragma mark -
#pragma mark Action Creation Helpers
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorEnrollFace::CreateTurnTowardsFaceAction(Vision::FaceID_t faceID,
                                                               Vision::FaceID_t saveID,
                                                               bool playScanningGetOut)
{
  CompoundActionParallel* liftAndTurnTowardsAction = new CompoundActionParallel({
    new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK)
  });
  
  if(playScanningGetOut)
  {
    // If we we are enrolling, we need to get out of the "scanning face" animation while
    // doing this
    liftAndTurnTowardsAction->AddAction(new TriggerAnimationAction(AnimationTrigger::MeetCozmoLookFaceInterrupt));
  }
  
  if(!CanMoveTreads())
  {
    // If being held in the air, don't try to turn, so just return the parallel
    // compound action as it is now
    return liftAndTurnTowardsAction;
  }
  
  IActionRunner* turnAction = nullptr;
  
  if(faceID != Vision::UnknownFaceID)
  {
    // Try to look at the specified face
    const Vision::TrackedFace* face = GetBEI().GetFaceWorld().GetFace(faceID);
    if(nullptr != face) {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.TurningTowardsFaceID",
                    "Turning towards faceID=%d (saveID=%d)",
                    faceID, saveID);

      SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(faceID);
      turnAction = new TurnTowardsFaceAction(smartID, DEG_TO_RAD(45.f));
    }
  }
  else
  {
    // We weren't given a specific FaceID, so grab the last observed *unnamed*
    // face ID from FaceWorld and look at it (if there is one). If saveID was
    // specified, check first to see it's present in FaceWorld and turn towards
    // it (since that's who we are re-enrolling).
    const Vision::TrackedFace* faceToTurnTowards = nullptr;
    if(saveID != Vision::UnknownFaceID )
    {
      faceToTurnTowards = GetBEI().GetFaceWorld().GetFace(saveID);
    }
    
    if(faceToTurnTowards == nullptr)
    {
      auto allFaceIDs = GetBEI().GetFaceWorld().GetFaceIDs();
      for(auto & ID : allFaceIDs)
      {
        const Vision::TrackedFace* face = GetBEI().GetFaceWorld().GetFace(ID);
        if(ANKI_VERIFY(face != nullptr, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.NullFace", "ID:%d", ID))
        {
          if(!face->HasName())
          {
            // Use this face if:
            // - faceToTurnTowards hasn't been set yet, OR
            // - this face is newer than the curent face to turn towards, OR
            // - this face was seen at the same time as the current face to
            //    turn towards and we win coin toss (to randomly break ties)
            if((faceToTurnTowards == nullptr) ||
               (face->GetTimeStamp() > faceToTurnTowards->GetTimeStamp()) ||
               (face->GetTimeStamp() == faceToTurnTowards->GetTimeStamp() &&
                GetBEI().GetRNG().RandDbl() < 0.5))
            {
              faceToTurnTowards = face;
            }
          }
        }
      }
    }
    
    if(nullptr != faceToTurnTowards)
    {
      // Couldn't find face in face world, try turning towards last face pose
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.FoundFace",
                    "Turning towards faceID=%d last seen at t=%d (saveID=%d)",
                    faceToTurnTowards->GetID(), faceToTurnTowards->GetTimeStamp(), saveID);

      SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(faceToTurnTowards->GetID());
      turnAction = new TurnTowardsFaceAction(smartID, DEG_TO_RAD(90.f));
    }
  }
  
  if(turnAction == nullptr)
  {
    // Couldn't find face in face world, try turning towards last face pose
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.NullFace",
                  "No face found to turn towards. FaceID=%d. SaveID=%d. Turning towards last face pose.",
                  faceID, saveID);
    
    // No face found to look towards: fallback on looking at last face pose
    turnAction = new TurnTowardsLastFacePoseAction(DEG_TO_RAD(45.f));
  }
  
  // Add whatever turn action we decided to create to the parallel action and return it
  liftAndTurnTowardsAction->AddAction(turnAction);
  
  return liftAndTurnTowardsAction;
  
} // CreateTurnTowardsFaceAction()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorEnrollFace::CreateLookAroundAction()
{
  // If we haven't seen the face since this behavior was created,
  // try looking up further: it's more likely a face is further up and
  // we're looking too low. Add a little movement so he doesn't look dead.
  // NOTE: we will just keep doing this until timeout if we never see the face!
  const Radians absHeadAngle = GetRNG().RandDblInRange(MAX_HEAD_ANGLE - DEG_TO_RAD(10), MAX_HEAD_ANGLE);
  
  // Rotate in the opposite direction enough to undo the last rotation plus a little more
  const double newAngle = std::copysign(GetRNG().RandDblInRange(0, DEG_TO_RAD(10)),
                                        -_lastRelBodyAngle.ToDouble());
  const Radians relBodyAngle = newAngle -_lastRelBodyAngle;
  _lastRelBodyAngle = newAngle;

  CompoundActionSequential* compoundAction = new CompoundActionSequential();
  
  if(CanMoveTreads())
  {
    compoundAction->AddAction(new PanAndTiltAction(relBodyAngle, absHeadAngle, false, true));
    
    // Also back up a little if we haven't gone too far back already
    if(_totalBackup_mm <= kEnrollFace_MaxTotalBackup_mm)
    {
      const f32 backupSpeed_mmps = 100.f;
      const f32 backupDist_mm = GetRNG().RandDblInRange(kEnrollFace_MinBackup_mm, kEnrollFace_MaxBackup_mm);
      _totalBackup_mm += backupDist_mm;
      const bool shouldPlayAnimation = false; // don't want head to move down!
      DriveStraightAction* backUpAction = new DriveStraightAction(-backupDist_mm, backupSpeed_mmps, shouldPlayAnimation);
      compoundAction->AddAction(backUpAction);
    }
  }
  else
  {
    // If in the air (i.e. held in hand), just move head, not body
    compoundAction->AddAction(new MoveHeadToAngleAction(absHeadAngle));
  }
  
  compoundAction->AddAction(new WaitForImagesAction(kEnrollFace_NumImagesToWait, VisionMode::DetectingFaces));
  
  return compoundAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::UpdateFaceIDandTime(const Face* newFace)
{
  DEV_ASSERT(nullptr != newFace, "BehaviorEnrollFace.UpdateFaceToEnroll.NullNewFace");
  _faceID = newFace->GetID();
  _lastFaceSeenTime_ms = newFace->GetTimeStamp();
  
  _observedUnusableName.clear();
  _observedUnusableID = Vision::UnknownFaceID;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsSeeingTooManyFaces(FaceWorld& faceWorld, const TimeStamp_t lastImgTime)
{
  // Check if we've also seen too many within a recent time window
  const TimeStamp_t multipleFaceTimeWindow_ms = Util::SecToMilliSec(_tooManyFacesRecentTime_sec);
  const TimeStamp_t recentTime = (lastImgTime > multipleFaceTimeWindow_ms ?
                                  lastImgTime - multipleFaceTimeWindow_ms :
                                  0); // Avoid unsigned math rollover
  
  auto recentlySeenFaceIDs = faceWorld.GetFaceIDsObservedSince(recentTime);
  
  const bool hasRecentlySeenTooManyFaces = recentlySeenFaceIDs.size() > _maxFacesVisible;
  if(hasRecentlySeenTooManyFaces)
  {
    if(_startedSeeingMultipleFaces_sec == 0.f)
    {
      // We just started seeing too many faces
      _startedSeeingMultipleFaces_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      // Disable enrollment while seeing too many faces
      faceWorld.Enroll(Vision::UnknownFaceID);
      
      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StartedSeeingTooMany",
                     "Disabling enrollment (if enabled) at t=%.1f", _startedSeeingMultipleFaces_sec);
    }
    return true;
  }
  else
  {
    if(_startedSeeingMultipleFaces_sec > 0.f)
    {
      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StoppedSeeingTooMany",
                     "Stopped seeing too many at t=%.1f", _startedSeeingMultipleFaces_sec);
      
      // We are not seeing too many faces any more (and haven't recently), so reset this to zero
      _startedSeeingMultipleFaces_sec = 0.f;
      
      if(_faceID != Vision::UnknownFaceID)
      {
        // Re-enable enrollment of whatever we were enrolling before we started seeing too many faces
        faceWorld.Enroll(_faceID);
        
        PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.RestartEnrollment",
                       "Re-enabling enrollment of FaceID:%d", _faceID);
        
      }
    }
    return false;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::UpdateFaceToEnroll()
{
  const FaceWorld& faceWorld = GetBEI().GetFaceWorld();
  auto& robotInfo = GetBEI().GetRobotInfo();

  const TimeStamp_t lastImgTime = robotInfo.GetLastImageTimeStamp();
  
  const bool tooManyFaces = IsSeeingTooManyFaces(GetBEI().GetFaceWorldMutable(), lastImgTime);
  if(tooManyFaces)
  {
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.UpdateFaceToEnroll.TooManyFaces", "");
    // Early return here will prevent "lastFaceSeenTime" from being updated, eventually causing us
    // to transition out of Enrolling state, back to LookingForFace, if necessary. If we are already LookingForFace,
    // we will time out.
    return;
  }
  
  // Get faces observed just in the last image
  auto observedFaceIDs = faceWorld.GetFaceIDsObservedSince(lastImgTime);
  
  const bool enrollmentIDisSet    = (_faceID != Vision::UnknownFaceID);
  const bool sawCurrentEnrollFace = (enrollmentIDisSet && observedFaceIDs.count(_faceID));
  
  if(sawCurrentEnrollFace)
  {
    // If we saw the face we're currently enrolling, there's nothing to do other than
    // update it's last seen time
    const Face* enrollFace = GetBEI().GetFaceWorld().GetFace(_faceID);
    UpdateFaceIDandTime(enrollFace);
  }
  else
  {
    // Didn't see current face (or don't have one yet). Look at others and see if
    // we want to switch to any of them.
    for(auto faceID : observedFaceIDs)
    {
      // We just checked that _faceID *wasn't* seen!
      DEV_ASSERT(faceID != _faceID, "BehaviorEnrollFace.UpdateFaceToEnroll.UnexpectedFaceID");
      
      // We only care about this observed face if it is not for a "tracked" face
      // (one with negative ID, which we never want to try to enroll)
      if(faceID <= 0)
      {
        PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.UpdateFaceToEnroll.SkipTrackedFace",
                       "Skipping tracking-only ID:%d", faceID);
        continue;
      }
      
      const Face* newFace = GetBEI().GetFaceWorld().GetFace(faceID);
      if(nullptr == newFace)
      {
        PRINT_NAMED_WARNING("BehaviorEnrollFace.UpdateFaceToEnroll.NullFace",
                            "FaceID %d cam back null", faceID);
        continue;
      }
      
      // Record the last person we saw (that we weren't already enrolling), in
      // case we fail and need to message that the reason why was that we we were
      // seeing this named face. These get cleared if we end up using this observed
      // face for enrollment.
      _observedUnusableID   = faceID;
      _observedUnusableName = newFace->GetName();
      
      // We can only switch to this observed faceID if it is unnamed, _unless_
      // it matches the saveID.
      // - for new enrollments we can't enroll an already-named face (that's a re-enrollment, by definition)
      // - for re-enrollment, a face with a name must be the one we are re-enrolling
      // - if the name matches the face ID, then the faceID matches too and we wouldn't even
      //   be considering this observation because there's no ID change
      const bool canUseObservedFace = !newFace->HasName() || (faceID == _saveID);
      
      if(canUseObservedFace)
      {
        if(enrollmentIDisSet)
        {
          // Face ID is already set but we didn't see it and instead we're seeing a face
          // with a different ID. See if it matches the pose of the one we were already enrolling.
          
          auto currentFace = GetBEI().GetFaceWorld().GetFace(_faceID);
          
          if(nullptr != currentFace && nullptr != newFace &&
             newFace->GetHeadPose().IsSameAs(currentFace->GetHeadPose(),
                                             kEnrollFace_UpdateFacePositionThreshold_mm,
                                             DEG_TO_RAD(kEnrollFace_UpdateFaceAngleThreshold_deg)))
          {
            PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateFaceToEnroll.UpdatingFaceIDbyPose",
                          "Was enrolling ID=%d, changing to unnamed ID=%d based on pose (saveID=%d)",
                          _faceID, faceID, _saveID);
            
            UpdateFaceIDandTime(newFace);
          }
          
        }
        else
        {
          // We don't have a face ID set yet. Use this one, since it passed all the earlier checks
          PRINT_CH_INFO(kLogChannelName,
                        "BehaviorEnrollFace.UpdateFaceToEnroll.SettingInitialFaceID",
                        "Set face ID to unnamed face %d (saveID=%d)", faceID, _saveID);
          
          UpdateFaceIDandTime(newFace);
        }
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName,
                      "BehaviorEnrollFace.UpdateFaceToEnroll.IgnoringObservedFace",
                      "Refusing to enroll '%s' face %d, with current faceID=%d and saveID=%d",
                      !newFace->HasName() ? "<unnamed>" : Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()),
                      faceID, _faceID, _saveID);
      }
      
    } // for each face ID
    

  } // if/else(sawCurrentEnrollFace)
  
} // UpdateFaceToEnroll()
  
#pragma mark -
#pragma mark Event Handlers
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::RobotChangedObservedFaceID:
    {
      auto const& msg = event.GetData().Get_RobotChangedObservedFaceID();
      
      // Listen for changed ID messages in case the FaceRecognizer changes the ID we
      // were enrolling
      if(msg.oldID == _faceID)
      {
        const Vision::TrackedFace* newFace = GetBEI().GetFaceWorld().GetFace(msg.newID);
        if(msg.newID != _saveID &&
           newFace != nullptr &&
           newFace->HasName())
        {
          // If we just realized the faceID we were enrolling is someone else and that
          // person is already enrolled with a name, we should abort (unless the newID
          // matches the person we were re-enrolling).
          PRINT_CH_INFO(kLogChannelName,
                        "BehaviorEnrollFace.HandleRobotChangedObservedFaceID.CannotUpdateToNamedFace",
                        "OldID:%d. NewID:%d is named '%s' and != SaveID:%d, so cannot be used",
                        msg.oldID, msg.newID,
                        Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()), _saveID);
          
          TransitionToFailedState(State::Failed_WrongFace, "Failed_WrongFace");
          
          _observedUnusableID   = msg.newID;
          _observedUnusableName = newFace->GetName();
        }
        else
        {
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleRobotChangedObservedFaceID.UpdatingFaceID",
                        "Was enrolling ID=%d, changing to ID=%d",
                        _faceID, msg.newID);
          _faceID = msg.newID;
        }
      }
      
      if(msg.oldID == _saveID)
      {
        // I don't think this should happen: we should never update a saveID because it
        // should be named, meaning we should never merge into it
        PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleRobotChangedObservedFaceID.SaveIDChanged",
                          "Was saving to ID=%d, which apparently changed to %d. Should not happen. Will abort.",
                          _saveID, msg.newID);
        TransitionToFailedState(State::Failed_UnknownReason,"Failed_UnknownReason");
      }
      break;
    }
    case EngineToGameTag::UnexpectedMovement:
    case EngineToGameTag::RobotOffTreadsStateChanged:
    case EngineToGameTag::CliffEvent:
    {
      // Handled while running
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorEnrollFace.AlwaysHandle.UnexpectedEngineToGameTag",
                        "Received unexpected EngineToGame tag %s",
                        MessageEngineToGameTagToString(event.GetData().GetTag()));
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::RobotChangedObservedFaceID:
    {
      // Always handled
      break;
    }
    case EngineToGameTag::UnexpectedMovement:
    case EngineToGameTag::RobotOffTreadsStateChanged:
    case EngineToGameTag::CliffEvent:
    {
      if(IsControlDelegated())
      {
        if(State::LookingForFace == _state)
        {
          // Cancel and replace the existing look-around action so we don't fight a
          // person trying to re-orient Cozmo towards themselves, nor do we leave
          // the treads moving if a look around action had already started (but not
          // completed) before a pickup/cliff
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleWhileRunning.LookForFaceInterrupted",
                        "Restarting look-for-face due to %s event",
                        MessageEngineToGameTagToString(event.GetData().GetTag()));
          TransitionToLookingForFace();
        }
        else if(State::Enrolling == _state)
        {
          // Stop tracking the face and start over (to create a new tracking action, e.g. in case the robot is now
          // picked up and its treads should stop moving)
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleWhileRunning.EnrollmentInterrupted",
                        "Restarting enrollment due to %s event",
                        MessageEngineToGameTagToString(event.GetData().GetTag()));
          CancelDelegates(false);
          TransitionToEnrolling();
        }
      }
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleWhileRunning.UnexpectedEngineToGameTag",
                        "Received unexpected EngineToGame tag %s",
                        MessageEngineToGameTagToString(event.GetData().GetTag()));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::HandleWhileInScopeButNotActivated(const GameToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::SetFaceToEnroll:
    {
      auto & msg = event.GetData().Get_SetFaceToEnroll();
      if(msg.name.empty())
      {
        PRINT_NAMED_WARNING("BehaviorEnrollFace.HandleSetFaceToEnroll.EmptyName",
                            "Cannot enroll without a name specified. Ignoring request.");
        return;
      }
      
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleSetFaceToEnrollMessage",
                    "SaveID:%d ObsID:%d Name:%s",
                    msg.saveID, msg.observedID, Util::HidePersonallyIdentifiableInfo(msg.name.c_str()));
      
      *_settings = msg;
      break;
    }
      
    case GameToEngineTag::CancelFaceEnrollment:
    {
      // Handled while running
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleWhileNotRunning.IgnoringCancelEnrollment",
                    "Not running, ignoring cancellation message");
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleWhileNotRunning.UnexpectedGameToEngineTag",
                        "Received unexpected GameToEngine tag %s",
                        MessageGameToEngineTagToString(event.GetData().GetTag()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::HandleWhileActivated(const GameToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::SetFaceToEnroll:
    {
      // Handled while NOT running
      auto & msg = event.GetData().Get_SetFaceToEnroll();
      PRINT_NAMED_WARNING("BehaviorEnrollFace.HandleWhileRunning.IgnoringSetFaceToEnroll",
                          "Already enrolling, ignoring SetFaceToEnroll message with ID:%d SaveID:%d Name:%s",
                          msg.observedID, msg.saveID, Util::HidePersonallyIdentifiableInfo(msg.name.c_str()));
      break;
    }
      
    case GameToEngineTag::CancelFaceEnrollment:
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleCancelFaceEnrollmentMessage", "");
      SET_STATE(Cancelled);
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleWhileRunning.UnexpectedGameToEngineTag",
                        "Received unexpected GameToEngine tag %s",
                        MessageGameToEngineTagToString(event.GetData().GetTag()));
  }
}
  
} // namespace Cozmo
} // namespace Anki
