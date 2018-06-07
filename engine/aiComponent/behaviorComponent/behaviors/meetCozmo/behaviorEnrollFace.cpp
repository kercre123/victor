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
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/utils/timer.h"

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/trackedFace.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/enrolledFaceStorage.h"

#include "util/console/consoleInterface.h"

/* Note CPP files must explicitly include logging.h for correct expansion of DASMSG macros */
#include "util/logging/logging.h"

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

CONSOLE_VAR(u32,               kEnrollFace_TicksForKnownNameBeforeFail,         CONSOLE_GROUP, 35);
CONSOLE_VAR(TimeStamp_t,       kEnrollFace_TimeStopMovingAfterManualTurn_ms,    CONSOLE_GROUP, 8000);
CONSOLE_VAR(TimeStamp_t,       kEnrollFace_MaxInterruptionBeforeReset_ms,       CONSOLE_GROUP, 10000);

static const char * const kLogChannelName = "FaceRecognizer";
static const char * const kMaxFacesVisibleKey = "maxFacesVisible";
static const char * const kTooManyFacesTimeoutKey = "tooManyFacesTimeout_sec";
static const char * const kTooManyFacesRecentTimeKey = "tooManyFacesRecentTime_sec";

}

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _dVars.persistent.state = State::s; DEBUG_SET_STATE(s); } while(0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::InstanceConfig::InstanceConfig()
{
  timeout_sec = kEnrollFace_Timeout_sec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::DynamicVariables::DynamicVariables()
{
  // Settings ok: initialize rest of behavior state
  saveSucceeded = false;

  observedUnusableID  = Vision::UnknownFaceID;
  observedUnusableName.clear();

  startTime_sec       = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  timeScanningStarted_ms = 0;
  timeStartedLookingForFace_ms = 0;

  lastFaceSeenTime_ms            = 0;
  startedSeeingMultipleFaces_sec = 0.f;

  lastRelBodyAngle    = 0.f;
  totalBackup_mm      = 0.f;

  saveEnrollResult    = ActionResult::NOT_STARTED;
  saveAlbumResult     = ActionResult::NOT_STARTED;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::BehaviorEnrollFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  _dVars.persistent.settings.reset( new ExternalInterface::SetFaceToEnroll() );

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
  _iConfig.maxFacesVisible = config.get(kMaxFacesVisibleKey, kEnrollFace_DefaultMaxFacesVisible).asInt();
  _iConfig.tooManyFacesTimeout_sec = config.get(kTooManyFacesTimeoutKey, kEnrollFace_DefaultTooManyFacesTimeout_sec).asFloat();
  _iConfig.tooManyFacesRecentTime_sec = config.get(kTooManyFacesRecentTimeKey, kEnrollFace_DefaultTooManyFacesRecentTime_sec).asFloat();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.driveOffChargerBehavior.get() );
  delegates.insert( _iConfig.putDownBlockBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMaxFacesVisibleKey,
    kTooManyFacesTimeoutKey,
    kTooManyFacesRecentTimeKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::CheckForIntentData() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsUserIntentPending( USER_INTENT(meet_victor) ) ) {
    // activate the intent
    uic.ActivateUserIntent( USER_INTENT(meet_victor), GetDebugLabel() );
  }

  UserIntentPtr intentData = uic.GetUserIntentIfActive(USER_INTENT(meet_victor));
  if( intentData != nullptr ) {
    const auto& meetVictor = intentData->intent.Get_meet_victor();
    _dVars.persistent.settings->name = meetVictor.username;
    _dVars.persistent.settings->observedID = Vision::UnknownFaceID;
    _dVars.persistent.settings->saveID = 0;
    _dVars.persistent.settings->saveToRobot = true;
    _dVars.persistent.settings->sayName = true;
    _dVars.persistent.settings->useMusic = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = uic.IsUserIntentPending(USER_INTENT(meet_victor) );
  const bool isWaitingResume = (_dVars.persistent.state != State::NotStarted);

  const bool wantsToBeActivated = pendingIntent || isWaitingResume;
  return wantsToBeActivated;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEnrollFace::InitEnrollmentSettings()
{
  if( !IsEnrollmentRequested() ) {
    // this can happen in tests
    PRINT_NAMED_WARNING("BehaviorEnrollFace.InitEnrollmentSettings.FaceEnrollmentNotRequested",
                        "BehaviorEnrollFace started without an enrollment request");
  }

  _dVars.faceID        = _dVars.persistent.settings->observedID;
  _dVars.saveID        = _dVars.persistent.settings->saveID;
  _dVars.faceName      = _dVars.persistent.settings->name;
  _dVars.saveToRobot   = _dVars.persistent.settings->saveToRobot;
  _dVars.useMusic      = _dVars.persistent.settings->useMusic;
  _dVars.sayName       = _dVars.persistent.settings->sayName;

  _dVars.enrollingSpecificID = (_dVars.faceID != Vision::UnknownFaceID);

  if(_dVars.faceName.empty())
  {
    PRINT_NAMED_ERROR("BehaviorEnrollFace.InitEnrollmentSettings.EmptyName", "");
    return RESULT_FAIL;
  }

  if( GetBEI().GetVisionComponent().IsNameTaken( _dVars.faceName ) ) {
    TransitionToSayingIKnowThatName();
    return RESULT_FAIL;
  }

  // If saveID is specified and we've already seen it (so it's in FaceWorld), make
  // sure that it is the ID of a _named_ face
  if(_dVars.saveID != Vision::UnknownFaceID)
  {
    const Face* face = GetBEI().GetFaceWorld().GetFace(_dVars.saveID);
    if(nullptr != face && !face->HasName())
    {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.InitEnrollmentSettings.UnnamedSaveID",
                          "Face with SaveID:%d has no name", _dVars.saveID);
      return RESULT_FAIL;
    }
  }

  return RESULT_OK;
}

void BehaviorEnrollFace::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(DriveOffCharger) );
  _iConfig.putDownBlockBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(PutDownBlock) );
  ANKI_VERIFY( _iConfig.driveOffChargerBehavior != nullptr,
               "BehaviorEnrollFace.InitBehavior.NoDriveOffCharger",
               "Could not grab behavior ptr for drive off charger" );
  ANKI_VERIFY( _iConfig.putDownBlockBehavior != nullptr,
               "BehaviorEnrollFace.InitBehavior.NoPutDownCube",
               "Could not grab behavior ptr for put down cube" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::OnBehaviorActivated()
{
  CheckForIntentData();

  // reset dynamic variables
  {
    auto persistent = std::move(_dVars.persistent);
    _dVars = DynamicVariables();
    _dVars.persistent = std::move(persistent);
    _dVars.timeout_sec = _iConfig.timeout_sec;

    const auto& moveComp = GetBEI().GetMovementComponent();
    _dVars.wasUnexpectedRotationWithoutMotorsEnabled = moveComp.IsUnexpectedRotationWithoutMotorsEnabled();
  }

  const Result settingsResult = InitEnrollmentSettings();
  if(RESULT_OK != settingsResult)
  {
    PRINT_NAMED_WARNING("BehaviorEnrollFace.InitInternal.BadSettings",
                        "Disabling enrollment");
    if( _dVars.persistent.state != State::SayingIKnowThatName ) {
      CancelSelf();
    }
    return;
  }

  // Check if we were interrupted and need to fast forward:
  switch(_dVars.persistent.state)
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


  // Reset flag in FaceWorld because we're starting a new enrollment and will
  // be waiting for this new enrollment to be "complete" after this
  GetBEI().GetFaceWorldMutable().SetFaceEnrollmentComplete(false);

  // Make sure enrollment is enabled for session-only faces when we start. Otherwise,
  // we won't even be able to start enrollment because everything will remain a
  // "tracking only" face.
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);

  PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal",
                "Initialize with ID=%d and name '%s', to be saved to ID=%d",
                _dVars.faceID, Util::HidePersonallyIdentifiableInfo(_dVars.faceName.c_str()), _dVars.saveID);

  if( GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    TransitionToDriveOffCharger();
  } else if( GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() ) {
    TransitionToPutDownBlock();
  } else {
    // First thing we want to do is turn towards the face and make sure we see it
    TransitionToLookingForFace();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::BehaviorUpdate()
{
  // conditions that would end enrollment, even if the behavior has been interrupted
  if( (_dVars.persistent.settings != nullptr) && IsEnrollmentRequested() ) {
    const bool lowBattery = GetBEI().GetRobotInfo().GetBatteryLevel() == BatteryLevel::Low;
    const auto& uic = GetBehaviorComp<UserIntentComponent>();
    const bool triggerWordPending = uic.IsTriggerWordPending();
    if( lowBattery || triggerWordPending ) {
      DisableEnrollment();
      SET_STATE( NotStarted );
      return;
    }
  }

  if( !IsActivated() ) {
    if( _dVars.persistent.state != State::NotStarted ) {
      // interrupted
      if( _dVars.persistent.lastDeactivationTime_ms > 0 ) {
        TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        if( currTime_ms - _dVars.persistent.lastDeactivationTime_ms > kEnrollFace_MaxInterruptionBeforeReset_ms ) {
          DisableEnrollment();
          SET_STATE( NotStarted );
        }
      }
    }
    return;
  }

  // See if we were in the midst of finding or enrolling a face but the enrollment is
  // no longer requested, then we've been cancelled
  if((State::LookingForFace == _dVars.persistent.state || State::Enrolling == _dVars.persistent.state) && !IsEnrollmentRequested())
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateInternal_Legacy.EnrollmentCancelled", "In state: %s",
                  _dVars.persistent.state == State::LookingForFace ? "LookingForFace" : "Enrolling");
    CancelSelf();
    return;
  }


  if( !GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    _dVars.persistent.didEverLeaveCharger = true;
  }

  const bool justPlacedOnCharger = _dVars.persistent.didEverLeaveCharger && GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( justPlacedOnCharger && (_dVars.persistent.state != State::Enrolling) ) {
    // user placed the robot on the charger. cancel. Don't cancel for Enrolling since that needs
    // a getout and is handled below
    SET_STATE(Cancelled);
  }

  switch(_dVars.persistent.state)
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
    case State::DriveOffCharger:
    case State::PutDownBlock:
    {
      // Nothing specific to do: just waiting for animation/save to complete
      break;
    }

    case State::Enrolling:
    {
      bool finishedScanning = false;
      // Check to see if we're done
      if(GetBEI().GetFaceWorld().IsFaceEnrollmentComplete())
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CheckIfDone.ReachedEnrollmentCount", "");

        finishedScanning = true;

        // If we complete successfully, unset the observed ID/name
        _dVars.observedUnusableID = Vision::UnknownFaceID;
        _dVars.observedUnusableName.clear();
        GetBEI().GetVisionComponent().AssignNameToFace(_dVars.faceID, _dVars.faceName, _dVars.saveID);

        // Note that we will wait to disable face enrollment until the very end of
        // the behavior so that we remain resume-able from reactions, in case we
        // are interrupted after this point (e.g. while playing the sayname animations).

        TransitionToSayingName();
      }
      else if(HasTimedOut() || justPlacedOnCharger)
      {
        finishedScanning = true;
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
        if(robotInfo.GetLastImageTimeStamp() - _dVars.lastFaceSeenTime_ms > kEnrollFace_TimeoutForReLookForFace_ms)
        {
          _dVars.lastFaceSeenTime_ms = 0;

          // Go back to looking for face
          PRINT_NAMED_INFO("BehaviorEnrollFace.CheckIfDone.NoLongerSeeingFace",
                           "Have not seen face %d in %dms, going back to LookForFace state for %s",
                           _dVars.faceID, kEnrollFace_TimeoutForReLookForFace_ms,
                           _dVars.enrollingSpecificID ? ("face " + std::to_string(_dVars.faceID)).c_str() : "any face");

          // If we are not enrolling a specific face ID, we are allowed to try again
          // with a new face, so don't hang waiting to see the one we previously picked
          if(!_dVars.enrollingSpecificID)
          {
            _dVars.faceID = Vision::UnknownFaceID;
          }

          TransitionToLookingForFace();
        }
      }

      if( finishedScanning ) {
        // tell the app we've finished scanning
        if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
          ExternalInterface::MeetVictorFaceScanComplete status;
          GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(status)));
        }
        // das msg
        {
          const TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
          const TimeStamp_t timeSpentScanning_ms = currentTime_ms - _dVars.timeScanningStarted_ms;
          const TimeStamp_t timeBeforeFirstFace_ms = _dVars.timeScanningStarted_ms - _dVars.timeStartedLookingForFace_ms;
          int numPartialFacesSeen = 0;
          int numFullFacesSeen = 0;
          int numNamedFacesSeen = 0;
          for( const auto& faceID : _dVars.facesSeen ) {
            const auto it = _dVars.isFaceNamed.find(faceID);
            if( it != _dVars.isFaceNamed.end() && it->second ) {
              ++numNamedFacesSeen;
            }
            if( faceID > 0 ) {
              ++numFullFacesSeen;
            } else if( faceID < 0 ) {
              ++numPartialFacesSeen;
            }
          }

          DASMSG(behavior_meet_victor_scan_end, "behavior.meet_victor.scan_end", "Face scanning ended in meet victor");
          DASMSG_SET(i1, timeSpentScanning_ms, "Time spent scanning faces (ms)");
          DASMSG_SET(i2, timeBeforeFirstFace_ms, "Time scanning before seeing the first face (ms)");
          DASMSG_SEND();

          DASMSG(behavior_meet_victor_scan_faces, "behavior.meet_victor.scan_faces", "Info about # of faces seen when scanning, sent at scan_end");
          DASMSG_SET(i1, numPartialFacesSeen, "Number of partial faces seen during scanning");
          DASMSG_SET(i2, numFullFacesSeen, "Number of full faces seen during scanning");
          DASMSG_SET(i3, numNamedFacesSeen, "Number of named faces seen during scanning");
          DASMSG_SEND();
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
  _dVars.persistent.lastDeactivationTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  auto& moveComp = GetBEI().GetMovementComponent();
  moveComp.EnableUnexpectedRotationWithoutMotors( _dVars.wasUnexpectedRotationWithoutMotorsEnabled );

  const auto& robotInfo = GetBEI().GetRobotInfo();
  // if on the charger, we're exiting to the on charger reaction, unity is going to try to cancel but too late.
  if( robotInfo.IsOnChargerContacts() )
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.CancelBecauseOnCharger","");
    SET_STATE(Cancelled);
  }

  ExternalInterface::FaceEnrollmentCompleted info;

  if( _dVars.persistent.state == State::EmotingConfusion ) {
    // interrupted while in a transient animation state. Replace with the reason for being
    // in this state
    _dVars.persistent.state = _dVars.failedState;
  }

  if( ANKI_DEVELOPER_CODE ) {
    // in unit tests, this behavior will always want to re-activate when Cancel via the delegation component,
    // unless we disable enrollment. Use a special name (one that would almost certainly never be spoken)
    if( _dVars.faceName == "Special name for unit tests to end enrollment" ) {
      SET_STATE(Success);
      DisableEnrollment();
    }
  }

  const bool wasSeeingMultipleFaces = _dVars.startedSeeingMultipleFaces_sec > 0.f;
  const bool observedUnusableFace = _dVars.observedUnusableID != Vision::UnknownFaceID && !_dVars.observedUnusableName.empty();

  // If observed ID/face are set, then it means we never found a valid, unnamed face to use
  // for enrollment, so return those in the completion message and indicate this in the result.
  // NOTE: Seeing multiple faces effectively takes precedence here.
  if(_dVars.persistent.state == State::Failed_WrongFace || (_dVars.persistent.state == State::TimedOut &&
                                           !wasSeeingMultipleFaces &&
                                           observedUnusableFace))
  {
    info.faceID = _dVars.observedUnusableID;
    info.name   = _dVars.observedUnusableName;
    info.result = FaceEnrollmentResult::SawWrongFace;
  }
  else
  {
    if(_dVars.saveID != Vision::UnknownFaceID)
    {
      // We just merged the enrolled ID (faceID) into saveID, so report saveID as
      // "who" was enrolled
      info.faceID = _dVars.saveID;
    }
    else
    {
      info.faceID = _dVars.faceID;
    }

    info.name   = _dVars.faceName;

    switch(_dVars.persistent.state)
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

      case State::DriveOffCharger:
      case State::PutDownBlock:
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

  int numInterruptions = _dVars.persistent.numInterruptions;

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
    const bool isNewEnrollment = Vision::UnknownFaceID != _dVars.faceID && Vision::UnknownFaceID == _dVars.saveID;
    if(info.result != FaceEnrollmentResult::Success && isNewEnrollment)
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.ErasingNewlyEnrolledFace",
                    "Erasing new face %d as a precaution because we are about to report failure result: %s",
                    _dVars.faceID, EnumToString(info.result));
      GetBEI().GetVisionComponent().EraseFace(_dVars.faceID);
    }

    if(info.result == FaceEnrollmentResult::Success)
    {
      BehaviorObjectiveAchieved(BehaviorObjective::EnrolledFaceWithName);
    }

    // Log enrollment to DAS, with result type
    Util::sInfoF("robot.face_enrollment", {{DDATA, EnumToString(info.result)}}, "%d", info.faceID);

    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.BroadcastCompletion",
                  "In state:%hhu, FaceEnrollmentResult=%s",
                  _dVars.persistent.state, EnumToString(info.result));

    if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
      GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( ExternalMessageRouter::Wrap(std::move(info)) );
    }

    // Done (whether success or failure), so reset state for next run
    SET_STATE(NotStarted);
  } else {
    numInterruptions = ++_dVars.persistent.numInterruptions;
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsUserIntentActive( USER_INTENT(meet_victor) ) ) {
    uic.DeactivateUserIntent( USER_INTENT(meet_victor) );
  }

  if( info.result == FaceEnrollmentResult::Success ) {
    GetAIComp<AIWhiteboard>().OfferPostBehaviorSuggestion( PostBehaviorSuggestions::Socialize );
  }

  {
    DASMSG(behavior_meet_victor_end, "behavior.meet_victor.end", "Meet victor completed");
    DASMSG_SET(s1,
               FaceEnrollmentResultToString(info.result),
               "Completion status (Success,SawWrongFace,SawMultipleFaces,TimedOut,SaveFailed,Incomplete,Cancelled,NameInUse,UnknownFailure)");
    DASMSG_SET(i1, info.faceID, "faceID, if applicable");
    DASMSG_SET(i2, numInterruptions, "number of interruptions (so far [if Incomplete], or total otherwise])");
    DASMSG_SEND();
  }

  PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.StopInternal.FinalState",
                 "Stopping EnrollFace in state %s", GetDebugStateName().c_str());
}

#pragma mark -
#pragma mark State Machine

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsEnrollmentRequested() const
{
  const bool hasName = !_dVars.persistent.settings->name.empty();
  return hasName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::DisableEnrollment()
{
  _dVars.persistent.settings->name.clear();
  _dVars.persistent.didEverLeaveCharger = false;
  _dVars.persistent.lastTimeUserMovedRobot = 0;
  _dVars.persistent.lastDeactivationTime_ms = 0;
  _dVars.persistent.numInterruptions = 0;
  // Leave "session-only" face enrollment enabled when we finish
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::HasTimedOut() const
{
  const f32 currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasTimedOut = (currTime_sec > _dVars.startTime_sec + _dVars.timeout_sec);
  const bool hasSeenTooManyFacesTooLong = (_dVars.startedSeeingMultipleFaces_sec > 0.f &&
                                           (currTime_sec > _dVars.startedSeeingMultipleFaces_sec + _iConfig.tooManyFacesTimeout_sec));

  if(hasTimedOut)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.BehaviorTimedOut",
                  "TimedOut after %.1fsec in State:%s",
                  _dVars.timeout_sec, GetDebugStateName().c_str());
  }

  if(hasSeenTooManyFacesTooLong)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.TooManyFacesTooLong",
                  "Saw > %d faces for longer than %.1fsec in State:%s",
                  _iConfig.maxFacesVisible, _iConfig.tooManyFacesTimeout_sec, GetDebugStateName().c_str());
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
void BehaviorEnrollFace::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);

  auto next = [this](){
    if( GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() ) {
      TransitionToPutDownBlock();
    } else {
      TransitionToLookingForFace();
    }
  };

  if( _iConfig.driveOffChargerBehavior->WantsToBeActivated() ) {
    DelegateNow( _iConfig.driveOffChargerBehavior.get(), next);
  } else {
    next();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToPutDownBlock()
{
  SET_STATE(PutDownBlock);

  auto next = [this](){
    TransitionToLookingForFace();
  };

  if( _iConfig.putDownBlockBehavior->WantsToBeActivated() ) {
    DelegateNow( _iConfig.putDownBlockBehavior.get(), next);
  } else {
    next();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToLookingForFace()
{
  const bool playScanningGetOut = (State::Enrolling == _dVars.persistent.state);

  if( _dVars.timeStartedLookingForFace_ms == 0 ) {
    _dVars.timeStartedLookingForFace_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }

  // enable detection of unexpected rotation for the remainder of the behavior. This is reset
  // to its original value upon behavior deactivation. Note that ReactToUnexpectedMotion is suppressed
  // during this behavior
  auto& moveComp = GetBEI().GetMovementComponent();
  moveComp.EnableUnexpectedRotationWithoutMotors( true );

  SET_STATE(LookingForFace);

  IActionRunner* action = new CompoundActionSequential({
    CreateTurnTowardsFaceAction(_dVars.faceID, _dVars.saveID, playScanningGetOut),
    new WaitForImagesAction(kEnrollFace_NumImagesToWait, VisionMode::DetectingFaces),
  });


  // Make sure we stop tracking/scanning if necessary (CreateTurnTowardsFaceAction can create
  // a tracking action)
  CancelDelegates(false);

  DelegateIfInControl(action, [this]()
              {
                if(_dVars.lastFaceSeenTime_ms == 0)
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
                                  _dVars.faceID);

                    DelegateIfInControl(CreateLookAroundAction(),
                                        &BehaviorEnrollFace::TransitionToLookingForFace);
                  }
                }
                else
                {
                  // We've seen a face, so time to start enrolling it

                  // Give ourselves a little more time to finish now that we've seen a face, but
                  // don't go over the max timeout
                  _dVars.timeout_sec = std::min(kEnrollFace_TimeoutMax_sec, _dVars.timeout_sec + kEnrollFace_TimeoutExtraTime_sec);

                  PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.LookingForFace.FaceSeen",
                                "Found face %d to enroll. Timeout set to %.1fsec",
                                _dVars.faceID, _dVars.timeout_sec);

                  auto getInAnimAction = new TriggerAnimationAction(AnimationTrigger::MeetVictorGetIn);

                  IActionRunner* action = nullptr;
                  if(CanMoveTreads())
                  {
                    SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(_dVars.faceID);
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
                    GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( ExternalMessageRouter::Wrap(std::move(status)) );
                  }

                  {
                    DASMSG(behavior_meet_victor_scan_start, "behavior.meet_victor.scan_start", "Face scanning started in meet victor");
                    DASMSG_SEND();
                  }

                  DelegateIfInControl(action, &BehaviorEnrollFace::TransitionToEnrolling);
                }
              });

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToEnrolling()
{
  SET_STATE(Enrolling);

  _dVars.timeScanningStarted_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // Actually enable directed enrollment of the selected face in the vision system
  GetBEI().GetFaceWorldMutable().Enroll(_dVars.faceID);

  TrackFaceAction* trackAction = new TrackFaceAction(_dVars.faceID);

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
  TriggerAnimationAction* scanLoop  = new TriggerAnimationAction(AnimationTrigger::MeetVictorLookFace, numLoops);

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

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::MeetVictorLookFaceInterrupt),
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

  CompoundActionSequential* finalAnimation = new CompoundActionSequential();

  if(_dVars.sayName)
  {
    if(_dVars.saveID == Vision::UnknownFaceID)
    {
      // If we're not being told which ID to save to, then assume this is a
      // first-time enrollment and play the bigger sequence of animations,
      // along with special music state
      // TODO: PostMusicState should take in a GameState::Music, making the cast unnecessary...
      if(_dVars.useMusic)
      {
        // NOTE: it will be up to the caller to stop this music
//        robot.GetRobotAudioClient()->PostMusicState((AudioMetaData::GameState::GenericState)AudioMetaData::GameState::Music::Minigame__Meet_Cozmo_Say_Name, false, 0);
      }

      {
        // 1. Say name once
        SayTextAction* sayNameAction1 = new SayTextAction(_dVars.faceName, SayTextIntent::Name_FirstIntroduction_1);
        sayNameAction1->SetAnimationTrigger(AnimationTrigger::MeetVictorSayName);
        finalAnimation->AddAction(sayNameAction1);
      }

      {
        // 2. Repeat name
        SayTextAction* sayNameAction2 = new SayTextAction(_dVars.faceName, SayTextIntent::Name_FirstIntroduction_2);
        sayNameAction2->SetAnimationTrigger(AnimationTrigger::MeetVictorSayNameAgain);
        finalAnimation->AddAction(sayNameAction2);
      }

    }
    else
    {
      // This is a re-enrollment
      SayTextAction* sayNameAction = new SayTextAction(_dVars.faceName, SayTextIntent::Name_Normal);
      sayNameAction->SetAnimationTrigger(AnimationTrigger::MeetVictorSayName);
      finalAnimation->AddAction(sayNameAction);
    }

    // This is kinda hacky, but we could have used up a lot of our timeout time
    // during enrollment and don't want to cutoff the final animation action (which could be
    // pretty long if it's a first time enrollment), so increase our timeout at this point.
    _dVars.timeout_sec += 30.f;
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
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      GetBEI().GetMoodManager().TriggerEmotionEvent("EnrolledNewFace", currTime_s);

      if(_dVars.saveToRobot)
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

  if(_dVars.sayName) {


    // todo: locale
    const std::string sentence = "eye already know a " + _dVars.faceName;
    SayTextAction* speakAction = new SayTextAction(sentence, SayTextIntent::Text);
    speakAction->SetAnimationTrigger(AnimationTrigger::MeetCozmoDuplicateName);

    SET_STATE(SayingIKnowThatName);
    DelegateIfInControl(speakAction, [this](ActionResult result) {
      SET_STATE(Failed_NameInUse);
    });
  } else {
    TransitionToFailedState(State::Failed_NameInUse,"Failed_NameInUse");
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToWrongFace( const std::string& faceName )
{
  SET_STATE(EmotingConfusion);

  _dVars.failedState = State::Failed_WrongFace;

  CancelDelegates(false);

  // todo: locale
  const std::string text = "youre    " + faceName;
  auto* sayNameAction = new SayTextAction(text, SayTextIntent::Name_FirstIntroduction_1);
  sayNameAction->SetAnimationTrigger(AnimationTrigger::MeetVictorSawWrongFace);

  DelegateIfInControl(sayNameAction, [this](ActionResult result) {
    if( ActionResult::SUCCESS != result ) {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToWrongFace.AnimFailed", "");
    }
    _dVars.persistent.state = State::Failed_WrongFace;
    SetDebugStateName("Failed_WrongFace");
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToFailedState( State state, const std::string& stateName )
{
  SET_STATE(EmotingConfusion);
  _dVars.failedState = state;

  CancelDelegates(false);

  auto* action = new TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorConfusion);

  DelegateIfInControl(action, [this, state, stateName](ActionResult result) {
    if( ActionResult::SUCCESS != result ) {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToFailedState.FinalAnimationFailed", "");
    }
    _dVars.persistent.state = state;
    SetDebugStateName(stateName);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( { VisionMode::DetectingFaces, EVisionUpdateFrequency::High } );
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
      _dVars.saveEnrollResult = ActionResult::SUCCESS;
    } else {
      _dVars.saveEnrollResult = ActionResult::ABORT;
    }
  };

  std::function<void(NVStorage::NVResult)> saveAlbumCallback = [this](NVStorage::NVResult result)
  {
    if(result == NVStorage::NVResult::NV_OKAY) {
      _dVars.saveAlbumResult = ActionResult::SUCCESS;
    } else {
      _dVars.saveAlbumResult = ActionResult::ABORT;
    }
  };

  GetBEI().GetVisionComponent().SaveFaceAlbumToRobot(saveAlbumCallback, saveEnrollCallback);

  std::function<bool(Robot& robot)> waitForSave = [this](Robot& robot) -> bool
  {
    // Wait for Save to complete (success or failure)
    if(ActionResult::NOT_STARTED != _dVars.saveEnrollResult &&
       ActionResult::NOT_STARTED != _dVars.saveAlbumResult)
    {
      if(ActionResult::SUCCESS == _dVars.saveEnrollResult &&
         ActionResult::SUCCESS == _dVars.saveAlbumResult)
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.TransitionToSavingToRobot.NVStorageSaveSucceeded", "");
        return true;
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToSavingToRobot.NVStorageSaveFailed",
                            "SaveEnroll:%s SaveAlbum:%s. Erasing face.",
                            EnumToString(_dVars.saveEnrollResult),
                            EnumToString(_dVars.saveAlbumResult));

        if(Vision::UnknownFaceID == _dVars.saveID)
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
    liftAndTurnTowardsAction->AddAction(new TriggerAnimationAction(AnimationTrigger::MeetVictorLookFaceInterrupt));
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
      for(auto& ID : allFaceIDs)
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

  const bool wasMovedRecently = WasMovedRecently();
  if( (turnAction == nullptr) && !wasMovedRecently )
  {
    // Couldn't find face in face world, try turning towards last face pose
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.NullFace",
                  "No face found to turn towards. FaceID=%d. SaveID=%d. Turning towards last face pose.",
                  faceID, saveID);

    // No face found to look towards: fallback on looking at last face pose
    turnAction = new TurnTowardsLastFacePoseAction(DEG_TO_RAD(45.f));
  }

  if( turnAction != nullptr ) {
    // Add whatever turn action we decided to create to the parallel action and return it
    liftAndTurnTowardsAction->AddAction(turnAction);
  }

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
                                        -_dVars.lastRelBodyAngle.ToDouble());
  const Radians relBodyAngle = newAngle -_dVars.lastRelBodyAngle;
  _dVars.lastRelBodyAngle = newAngle;

  CompoundActionSequential* compoundAction = new CompoundActionSequential();

  const bool wasMovedRecently = WasMovedRecently();
  if(CanMoveTreads() && !wasMovedRecently)
  {
    compoundAction->AddAction(new PanAndTiltAction(relBodyAngle, absHeadAngle, false, true));

    // Also back up a little if we haven't gone too far back already
    if(_dVars.totalBackup_mm <= kEnrollFace_MaxTotalBackup_mm)
    {
      const f32 backupSpeed_mmps = 100.f;
      const f32 backupDist_mm = GetRNG().RandDblInRange(kEnrollFace_MinBackup_mm, kEnrollFace_MaxBackup_mm);
      _dVars.totalBackup_mm += backupDist_mm;
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
  _dVars.faceID = newFace->GetID();
  _dVars.lastFaceSeenTime_ms = newFace->GetTimeStamp();

  _dVars.observedUnusableName.clear();
  _dVars.observedUnusableID = Vision::UnknownFaceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsSeeingTooManyFaces(FaceWorld& faceWorld, const TimeStamp_t lastImgTime)
{
  // Check if we've also seen too many within a recent time window
  const TimeStamp_t multipleFaceTimeWindow_ms = Util::SecToMilliSec(_iConfig.tooManyFacesRecentTime_sec);
  const TimeStamp_t recentTime = (lastImgTime > multipleFaceTimeWindow_ms ?
                                  lastImgTime - multipleFaceTimeWindow_ms :
                                  0); // Avoid unsigned math rollover

  auto recentlySeenFaceIDs = faceWorld.GetFaceIDsObservedSince(recentTime);

  for( const auto& faceID : recentlySeenFaceIDs ) {
    const Vision::TrackedFace* face = faceWorld.GetFace(faceID);
    // only save info on the face if it is known this tick, but don't remove saved faces
    if( nullptr != face ) {
      _dVars.facesSeen.insert( faceID );
      _dVars.isFaceNamed[faceID] = (faceID > 0) && face->HasName();
    }
  }


  const bool hasRecentlySeenTooManyFaces = recentlySeenFaceIDs.size() > _iConfig.maxFacesVisible;
  if(hasRecentlySeenTooManyFaces)
  {
    if(_dVars.startedSeeingMultipleFaces_sec == 0.f)
    {
      // We just started seeing too many faces
      _dVars.startedSeeingMultipleFaces_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

      // Disable enrollment while seeing too many faces
      faceWorld.Enroll(Vision::UnknownFaceID);

      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StartedSeeingTooMany",
                     "Disabling enrollment (if enabled) at t=%.1f", _dVars.startedSeeingMultipleFaces_sec);
    }
    return true;
  }
  else
  {
    if(_dVars.startedSeeingMultipleFaces_sec > 0.f)
    {
      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StoppedSeeingTooMany",
                     "Stopped seeing too many at t=%.1f", _dVars.startedSeeingMultipleFaces_sec);

      // We are not seeing too many faces any more (and haven't recently), so reset this to zero
      _dVars.startedSeeingMultipleFaces_sec = 0.f;

      if(_dVars.faceID != Vision::UnknownFaceID)
      {
        // Re-enable enrollment of whatever we were enrolling before we started seeing too many faces
        faceWorld.Enroll(_dVars.faceID);

        PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.RestartEnrollment",
                       "Re-enabling enrollment of FaceID:%d", _dVars.faceID);

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

  const bool unexpectedMovement = GetBEI().GetMovementComponent().IsUnexpectedMovementDetected();
  const bool pickedUp = robotInfo.IsPickedUp();
  if( unexpectedMovement || pickedUp ) {
    _dVars.persistent.lastTimeUserMovedRobot = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }

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

  const bool enrollmentIDisSet    = (_dVars.faceID != Vision::UnknownFaceID);
  const bool sawCurrentEnrollFace = (enrollmentIDisSet && observedFaceIDs.count(_dVars.faceID));

  if(sawCurrentEnrollFace)
  {
    // If we saw the face we're currently enrolling, there's nothing to do other than
    // update it's last seen time
    const Face* enrollFace = GetBEI().GetFaceWorld().GetFace(_dVars.faceID);
    UpdateFaceIDandTime(enrollFace);
  }
  else
  {
    // Didn't see current face (or don't have one yet). Look at others and see if
    // we want to switch to any of them.
    for(auto faceID : observedFaceIDs)
    {
      // We just checked that _faceID *wasn't* seen!
      DEV_ASSERT(faceID != _dVars.faceID, "BehaviorEnrollFace.UpdateFaceToEnroll.UnexpectedFaceID");

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
      _dVars.observedUnusableID   = faceID;
      _dVars.observedUnusableName = newFace->GetName();

      // We can only switch to this observed faceID if it is unnamed, _unless_
      // it matches the saveID.
      // - for new enrollments we can't enroll an already-named face (that's a re-enrollment, by definition)
      // - for re-enrollment, a face with a name must be the one we are re-enrolling
      // - if the name matches the face ID, then the faceID matches too and we wouldn't even
      //   be considering this observation because there's no ID change
      const bool canUseObservedFace = !newFace->HasName() || (faceID == _dVars.saveID);

      if(canUseObservedFace)
      {
        if(enrollmentIDisSet)
        {
          // Face ID is already set but we didn't see it and instead we're seeing a face
          // with a different ID. See if it matches the pose of the one we were already enrolling.

          auto currentFace = GetBEI().GetFaceWorld().GetFace(_dVars.faceID);

          if(nullptr != currentFace && nullptr != newFace &&
             newFace->GetHeadPose().IsSameAs(currentFace->GetHeadPose(),
                                             kEnrollFace_UpdateFacePositionThreshold_mm,
                                             DEG_TO_RAD(kEnrollFace_UpdateFaceAngleThreshold_deg)))
          {
            PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateFaceToEnroll.UpdatingFaceIDbyPose",
                          "Was enrolling ID=%d, changing to unnamed ID=%d based on pose (saveID=%d)",
                          _dVars.faceID, faceID, _dVars.saveID);

            UpdateFaceIDandTime(newFace);
          }

        }
        else
        {
          // We don't have a face ID set yet. Use this one, since it passed all the earlier checks
          PRINT_CH_INFO(kLogChannelName,
                        "BehaviorEnrollFace.UpdateFaceToEnroll.SettingInitialFaceID",
                        "Set face ID to unnamed face %d (saveID=%d)", faceID, _dVars.saveID);

          UpdateFaceIDandTime(newFace);
        }
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName,
                      "BehaviorEnrollFace.UpdateFaceToEnroll.IgnoringObservedFace",
                      "Refusing to enroll '%s' face %d, with current faceID=%d and saveID=%d",
                      !newFace->HasName() ? "<unnamed>" : Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()),
                      faceID, _dVars.faceID, _dVars.saveID);
        auto it = std::find_if( _dVars.knownFaceCounts.begin(), _dVars.knownFaceCounts.end(), [&](const auto& p) {
          return p.first == newFace->GetName();
        });
        if( it == _dVars.knownFaceCounts.end() ) {
          _dVars.knownFaceCounts.emplace_back( newFace->GetName(), 1 );
          it = _dVars.knownFaceCounts.end();
          --it;
        } else {
          ++it->second;
        }
        if( it->second >= kEnrollFace_TicksForKnownNameBeforeFail ) {
          TransitionToWrongFace( it->first );
          return;
        }
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
      if(msg.oldID == _dVars.faceID)
      {
        const Vision::TrackedFace* newFace = GetBEI().GetFaceWorld().GetFace(msg.newID);
        if(msg.newID != _dVars.saveID &&
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
                        Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()), _dVars.saveID);

          TransitionToWrongFace( newFace->GetName() );

          _dVars.observedUnusableID   = msg.newID;
          _dVars.observedUnusableName = newFace->GetName();
        }
        else
        {
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleRobotChangedObservedFaceID.UpdatingFaceID",
                        "Was enrolling ID=%d, changing to ID=%d",
                        _dVars.faceID, msg.newID);
          _dVars.faceID = msg.newID;
        }
      }

      if(msg.oldID == _dVars.saveID)
      {
        // I don't think this should happen: we should never update a saveID because it
        // should be named, meaning we should never merge into it
        PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleRobotChangedObservedFaceID.SaveIDChanged",
                          "Was saving to ID=%d, which apparently changed to %d. Should not happen. Will abort.",
                          _dVars.saveID, msg.newID);
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
      _dVars.persistent.lastTimeUserMovedRobot = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(IsControlDelegated())
      {
        if(State::LookingForFace == _dVars.persistent.state)
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
        else if(State::Enrolling == _dVars.persistent.state)
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

      *_dVars.persistent.settings = msg;
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

bool BehaviorEnrollFace::WasMovedRecently() const
{
  const TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  const TimeStamp_t timeSinceMoved_ms = currentTime_ms - _dVars.persistent.lastTimeUserMovedRobot;
  const bool movedRobotRecently = (timeSinceMoved_ms < kEnrollFace_TimeStopMovingAfterManualTurn_ms);
  return movedRobotRecently;
}

} // namespace Cozmo
} // namespace Anki
