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
// This class controls music state which is not relevant when playing audio on the robot
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
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/utils/timer.h"

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/trackedFace.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/enrolledFaceStorage.h"

#include "util/console/consoleInterface.h"

#include "util/logging/logging.h"
#include "util/logging/DAS.h"

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

// Max angle to turn while looking for a face
CONSOLE_VAR(f32,               kEnrollFace_MaxTurnTowardsFaceAngle_rad,         CONSOLE_GROUP, DEG_TO_RAD(180.f));
  
CONSOLE_VAR(s32,               kEnrollFace_NumImagesToWait,                     CONSOLE_GROUP, 5);
CONSOLE_VAR(s32,               kEnrollFace_NumImagesToWaitInPlace,              CONSOLE_GROUP, 25);

// Number of faces to consider "too many" and forced timeout when seeing that many
CONSOLE_VAR(s32,               kEnrollFace_DefaultMaxFacesVisible,              CONSOLE_GROUP, 1); // > this is "too many"
CONSOLE_VAR(f32,               kEnrollFace_DefaultTooManyFacesTimeout_sec,      CONSOLE_GROUP, 2.f);
CONSOLE_VAR(f32,               kEnrollFace_DefaultTooManyFacesRecentTime_sec,   CONSOLE_GROUP, 0.5f);

CONSOLE_VAR(u32,               kEnrollFace_TicksForKnownNameBeforeFail,         CONSOLE_GROUP, 35);
CONSOLE_VAR(TimeStamp_t,       kEnrollFace_MaxInterruptionBeforeReset_ms,       CONSOLE_GROUP, 10000);

// Whether seeing a named, wrong face causes the behavior to end. If not, will instead just go back
// to looking for a face
CONSOLE_VAR(bool,              kEnrollFace_FailOnWrongFace,                     CONSOLE_GROUP, false);
  
enum class SayWrongNameMode : u8
{
  Off   = 0,  // Don't say name at all, just go back to looking for faces
  Short = 1,  // Just say the name
  Long  = 2,  // "You are "X" not "Y"
};

// This only matters if kEnrollFace_FailOnWrongFace==false
CONSOLE_VAR(u8, kEnrollFace_SayWrongNameMode,  CONSOLE_GROUP, Util::EnumToUnderlying(SayWrongNameMode::Long));
  
  
static const char * const kLogChannelName = "FaceRecognizer";
static const char * const kMaxFacesVisibleKey = "maxFacesVisible";
static const char * const kTooManyFacesTimeoutKey = "tooManyFacesTimeout_sec";
static const char * const kTooManyFacesRecentTimeKey = "tooManyFacesRecentTime_sec";
static const char * const kFaceSelectionPenaltiesKey = "faceSelectionPenalties";

}

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _dVars->persistent.state = State::s; DEBUG_SET_STATE(s); } while(0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BehaviorEnrollFace::InstanceConfig
{
  InstanceConfig();
  
  s32              maxFacesVisible;
  f32              tooManyFacesRecentTime_sec;
  f32              tooManyFacesTimeout_sec;
  f32              timeout_sec;
  
  ICozmoBehaviorPtr driveOffChargerBehavior;
  ICozmoBehaviorPtr putDownBlockBehavior;
  std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;
  
  FaceSelectionComponent::FaceSelectionFactorMap faceSelectionCriteria;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BehaviorEnrollFace::DynamicVariables
{
  DynamicVariables();
  
  struct WrongFaceInfo {
    FaceID_t    faceID;
    u32         count;
    bool        idChanged;
    bool        nameSaid;
    
    explicit WrongFaceInfo(FaceID_t id, bool idJustChanged = false)
    : faceID(id)
    , count(1)
    , idChanged(idJustChanged)
    , nameSaid(false)
    { }
  };
  
  struct Persistent {
    State              state = State::NotStarted;
    bool               didEverLeaveCharger = false;
    EngineTimeStamp_t  lastDeactivationTime_ms = 0;
    
    bool               requestedRescan = false;
    
    using EnrollmentSettings = ExternalInterface::SetFaceToEnroll;
    std::unique_ptr<EnrollmentSettings> settings;
    
    std::map<std::string, WrongFaceInfo> wrongFaceStats;
    
    int numInterruptions = 0;
  };
  Persistent       persistent;
  
  bool             sayName;
  bool             useMusic;
  bool             saveToRobot;
  bool             saveSucceeded;
  bool             enrollingSpecificID;
  FaceID_t         faceID;
  FaceID_t         saveID;
  FaceID_t         observedUnusableID;
  
  RobotTimeStamp_t      lastFaceSeenTime_ms;
  
  EngineTimeStamp_t      timeScanningStarted_ms;
  EngineTimeStamp_t      timeStartedLookingForFace_ms;
  
  f32 timeout_sec;
  
  bool wasUnexpectedRotationWithoutMotorsEnabled;
  
  f32              startedSeeingMultipleFaces_sec;
  f32              startTime_sec;
  
  f32              totalBackup_mm;
  
  std::string      faceName;
  std::string      observedUnusableName;
  
  Radians          lastRelBodyAngle;
  
  std::set<Vision::FaceID_t> facesSeen;
  std::unordered_map<Vision::FaceID_t, bool> isFaceNamed;
  
  State            failedState;
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::InstanceConfig::InstanceConfig()
: timeout_sec(kEnrollFace_Timeout_sec)
, faceSelectionCriteria(FaceSelectionComponent::kDefaultSelectionCriteria)
{
  
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEnrollFace::BehaviorEnrollFace(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(new InstanceConfig)
, _dVars(new DynamicVariables)
{
  _dVars->persistent.settings.reset( new ExternalInterface::SetFaceToEnroll() );

  SubscribeToTags({EngineToGameTag::RobotChangedObservedFaceID});

  SubscribeToTags({{
    GameToEngineTag::SetFaceToEnroll,
    GameToEngineTag::CancelFaceEnrollment,
  }});

  // If Cozmo sees more than maxFacesVisible for longer than tooManyFacesTimeout seconds while looking for a face or
  // enrolling a face, then the behavior transitions to the TimedOut state and then returns the SawMultipleFaces
  // FaceEnrollmentResult.
  _iConfig->maxFacesVisible = config.get(kMaxFacesVisibleKey, kEnrollFace_DefaultMaxFacesVisible).asInt();
  _iConfig->tooManyFacesTimeout_sec = config.get(kTooManyFacesTimeoutKey, kEnrollFace_DefaultTooManyFacesTimeout_sec).asFloat();
  _iConfig->tooManyFacesRecentTime_sec = config.get(kTooManyFacesRecentTimeKey, kEnrollFace_DefaultTooManyFacesRecentTime_sec).asFloat();
  
  if( config.isMember(kFaceSelectionPenaltiesKey) ) {
    const bool parsedOK = FaceSelectionComponent::ParseFaceSelectionFactorMap(config[kFaceSelectionPenaltiesKey],
                                                                              _iConfig->faceSelectionCriteria);
    ANKI_VERIFY(parsedOK, "BehaviorEnrollFace.InvalidFaceSelectionConfig",
                "behavior '%s' has invalid config",
                GetDebugLabel().c_str());
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Needed b/c of forward declaration of iConfig and dVars
BehaviorEnrollFace::~BehaviorEnrollFace()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig->driveOffChargerBehavior.get() );
  delegates.insert( _iConfig->putDownBlockBehavior.get() );
  delegates.insert( _iConfig->ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMaxFacesVisibleKey,
    kTooManyFacesTimeoutKey,
    kTooManyFacesRecentTimeKey,
    kFaceSelectionPenaltiesKey,
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
    _dVars->persistent.settings->name = meetVictor.username;
    _dVars->persistent.settings->observedID = Vision::UnknownFaceID;
    _dVars->persistent.settings->saveID = 0;
    _dVars->persistent.settings->saveToRobot = true;
    _dVars->persistent.settings->sayName = true;
    _dVars->persistent.settings->useMusic = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = uic.IsUserIntentPending(USER_INTENT(meet_victor) );
  const bool isWaitingResume = (_dVars->persistent.state != State::NotStarted);
  const bool requestedRescan = _dVars->persistent.requestedRescan;

  const bool wantsToBeActivated = pendingIntent || isWaitingResume || requestedRescan;
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

  _dVars->faceID        = _dVars->persistent.settings->observedID;
  _dVars->saveID        = _dVars->persistent.settings->saveID;
  _dVars->faceName      = _dVars->persistent.settings->name;
  _dVars->saveToRobot   = _dVars->persistent.settings->saveToRobot;
  _dVars->useMusic      = _dVars->persistent.settings->useMusic;
  _dVars->sayName       = _dVars->persistent.settings->sayName;

  _dVars->enrollingSpecificID = (_dVars->faceID != Vision::UnknownFaceID);

  if(_dVars->faceName.empty())
  {
    PRINT_NAMED_ERROR("BehaviorEnrollFace.InitEnrollmentSettings.EmptyName", "");
    return RESULT_FAIL;
  }

  if(_dVars->saveID != Vision::UnknownFaceID)
  {
    // If saveID is specified and we've already seen it (so it's in FaceWorld), make
    // sure that it is the ID of a _named_ face
    const Face* face = GetBEI().GetFaceWorld().GetFace(_dVars->saveID);
    if(nullptr != face && !face->HasName())
    {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.InitEnrollmentSettings.UnnamedSaveID",
                          "Face with SaveID:%d has no name", _dVars->saveID);
      return RESULT_FAIL;
    }
  }
  else
  {
    // We're enrolling a new face. Make sure:
    // 1. The name is available
    // 2. We have room for a new face
    
    if(GetBEI().GetVisionComponent().IsNameTaken( _dVars->faceName )
       && !_dVars->enrollingSpecificID
       && (_dVars->persistent.state == State::NotStarted) )  // if previously interrupted after enrollment completed, this name is ok
    {
      TransitionToSayingIKnowThatName();
      return RESULT_FAIL;
    }
    
    if(!GetBEI().GetVisionComponent().CanAddNamedFace())
    {
      // If saveID is not specified, then we're trying to add a new face, so fail
      // if there's no room for new named faces
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitEnrollmentSettings.NoSpaceLeft", "");
      TransitionToFailedState(State::Failed_NamedStorageFull, "Failed_NamedStorageFull");
      return RESULT_FAIL;
    }
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig->driveOffChargerBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(DriveOffChargerStraight) );
  _iConfig->putDownBlockBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(PutDownBlock) );
  
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(DefaultTextToSpeechLoop),
                                  BEHAVIOR_CLASS(TextToSpeechLoop),
                                  _iConfig->ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::OnBehaviorActivated()
{
  CheckForIntentData();

  // reset dynamic variables
  {
    auto persistent = std::move(_dVars->persistent);
    _dVars.reset(new DynamicVariables());
    _dVars->persistent = std::move(persistent);
    _dVars->timeout_sec = _iConfig->timeout_sec;

    // This behavior uses a special form of unexpected movement detection. Store
    // current state of that mode (so we can put it back on deactivation) and then
    // enable for the duration of this behavior
    auto& moveComp = GetBEI().GetMovementComponent();
    _dVars->wasUnexpectedRotationWithoutMotorsEnabled = moveComp.IsUnexpectedRotationWithoutMotorsEnabled();
    moveComp.EnableUnexpectedRotationWithoutMotors( true );
  }

  // Check for special case interruption
  {
    const bool prevNameSet = !_dVars->persistent.settings->name.empty();
    const bool nameChanged = (_dVars->faceName != _dVars->persistent.settings->name);
    const bool interrupted = (_dVars->persistent.state != State::NotStarted);
    if(interrupted && prevNameSet && nameChanged)
    {
      // We were interrupted by a new enrollment. Just start the new enrollment from scratch
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.InitInternal.InterruptedByNewEnrollment",
                    "WasEnrolling %s, interrupted to enroll %s. Starting over.",
                    Util::HidePersonallyIdentifiableInfo(_dVars->persistent.settings->name.c_str()),
                    Util::HidePersonallyIdentifiableInfo(_dVars->faceName.c_str()));
      
      _dVars->persistent.state = State::NotStarted;
    }
  }
  
  const Result settingsResult = InitEnrollmentSettings();
  if(RESULT_OK != settingsResult)
  {
    PRINT_NAMED_WARNING("BehaviorEnrollFace.InitInternal.BadSettings",
                        "Disabling enrollment");
    if( _dVars->persistent.state != State::SayingIKnowThatName ) {
      CancelSelf();
    }
    return;
  }
  
  _dVars->persistent.requestedRescan = false;
  
  // Because we use SayTextAction instead of the tts coordinator for TTS, there's no way to do an
  // idle animation while TTS is being generated. Ideally we move animations in this behavior over
  // to the TTS coordinator, but that doesn't support audio keyframes yet. So instead, disable face
  // keepalive. This means that when the scanning loop ends and before the sayname action begins,
  // the eyes will retain the shape of the scanning loop's last frame for a few more ms. 
  SmartDisableKeepFaceAlive();

  // Check if we were interrupted and need to fast forward:
  switch(_dVars->persistent.state)
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
                _dVars->faceID, Util::HidePersonallyIdentifiableInfo(_dVars->faceName.c_str()), _dVars->saveID);

  if( GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    TransitionToDriveOffCharger();
  } else if( GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() ) {
    TransitionToPutDownBlock();
  } else {
    // First thing we want to do is turn towards the face and make sure we see it
    // todo (VIC-5000): use BehaviorFindFaceAndThen to inform the face search with person detection (nice ticket number, eh?)
    TransitionToLookingForFace();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::BehaviorUpdate()
{
  // conditions that would end enrollment, even if the behavior has been interrupted
  if( (_dVars->persistent.settings != nullptr) && IsEnrollmentRequested() ) {
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
    if( _dVars->persistent.state != State::NotStarted ) {
      // interrupted
      if( _dVars->persistent.lastDeactivationTime_ms > 0 ) {
        EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        if( currTime_ms - _dVars->persistent.lastDeactivationTime_ms > kEnrollFace_MaxInterruptionBeforeReset_ms ) {
          DisableEnrollment();
          SET_STATE( NotStarted );
        }
      }
    }
    return;
  }

  // See if we were in the midst of finding or enrolling a face but the enrollment is
  // no longer requested, then we've been cancelled
  if((State::LookingForFace == _dVars->persistent.state || State::Enrolling == _dVars->persistent.state) && !IsEnrollmentRequested())
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateInternal_Legacy.EnrollmentCancelled", "In state: %s",
                  _dVars->persistent.state == State::LookingForFace ? "LookingForFace" : "Enrolling");
    CancelSelf();
    return;
  }

  if( !GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    _dVars->persistent.didEverLeaveCharger = true;
  }

  const bool justPlacedOnCharger = _dVars->persistent.didEverLeaveCharger && GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( justPlacedOnCharger && (_dVars->persistent.state != State::Enrolling) ) {
    // user placed the robot on the charger. cancel. Don't cancel for Enrolling since that needs
    // a getout and is handled below
    SET_STATE(Cancelled);
  }

  switch(_dVars->persistent.state)
  {
    case State::Success:
    case State::NotStarted:
    case State::TimedOut:
    case State::Failed_WrongFace:
    case State::Failed_UnknownReason:
    case State::Failed_NameInUse:
    case State::Failed_NamedStorageFull:
    case State::SaveFailed:
    case State::Cancelled:
    {
      CancelSelf();
      return;
    }

    case State::WaitingInPlaceForFace:
    case State::LookingForFace:
    {
      // Check to see if the face we've been enrolling has changed based on what was
      // observed since the last tick
      UpdateFaceToEnroll();
      
      if( (_dVars->persistent.state == State::WaitingInPlaceForFace)
          && (_dVars->faceID != Vision::UnknownFaceID) )
      {
        CancelDelegates(false);
        // since the faceID is set, this next state should turn toward the face, then begin enrollment
        TransitionToLookingForFace();
        break;
      }
      
      // See if wrongFace info was updated via a changedID message or call to UpdateFaceToEnroll()
      std::string wrongName;
      FaceID_t wrongID = Vision::UnknownFaceID;
      if(IsSeeingWrongFace(wrongID, wrongName))
      {
        TransitionToWrongFace(wrongID, wrongName);
      }
      
      break;
    }
    
    case State::SayingName:
    case State::SayingIKnowThatName:
    case State::SayingWrongName:
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
        PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.BehaviorUpdate.ReachedEnrollmentCount", "");

        finishedScanning = true;

        // If we complete successfully, unset the observed ID/name
        _dVars->observedUnusableID = Vision::UnknownFaceID;
        _dVars->observedUnusableName.clear();
        GetBEI().GetVisionComponent().AssignNameToFace(_dVars->faceID, _dVars->faceName, _dVars->saveID);

        // Note that we will wait to disable face enrollment until the very end of
        // the behavior so that we remain resume-able from reactions, in case we
        // are interrupted after this point (e.g. while saving or playing the sayname animations).
        
        TransitionToSavingToRobot(); // will say name after saving
      }
      else if(HasTimedOut() || justPlacedOnCharger)
      {
        finishedScanning = true;
        // Need to play scanning get-out because we timed out while enrolling
        TransitionToScanningInterrupted();
      }
      else
      {
        const bool lostEnrollee = ((GetBEI().GetRobotInfo().GetLastImageTimeStamp()
                                    - _dVars->lastFaceSeenTime_ms) > kEnrollFace_TimeoutForReLookForFace_ms);
        
        // If we haven't seen the person (and only the one person) in too long, or got picked up
        // or reoriented, go back to looking for them
        if(lostEnrollee)
        {
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.BehaviorUpdate.LostEnrollee", "");
          finishedScanning = true;
          ResetEnrollment();
          TransitionToLookingForFace();
        }
        else
        {
          // Check to see if the face we've been enrolling has changed based on what was
          // observed since the last tick
          UpdateFaceToEnroll();
          
          // See if wrongFace info was updated via a changedID message or call to UpdateFaceToEnroll()
          std::string wrongName;
          FaceID_t wrongID = Vision::UnknownFaceID;
          if(IsSeeingWrongFace(wrongID, wrongName))
          {
            ResetEnrollment();
            TransitionToWrongFace(wrongID, wrongName);
          }
        }
      }

      if( finishedScanning )
      {
        // tell the app we've finished scanning
        if( GetBEI().GetRobotInfo().HasGatewayInterface() ) {
          auto* status = new external_interface::MeetVictorFaceScanComplete;
          GetBEI().GetRobotInfo().GetGatewayInterface()->Broadcast( ExternalMessageRouter::Wrap( status ) );
        }
        // das msg
        {
          const EngineTimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
          const TimeStamp_t timeSpentScanning_ms = (TimeStamp_t)(currentTime_ms - _dVars->timeScanningStarted_ms);
          const TimeStamp_t timeBeforeFirstFace_ms = (TimeStamp_t)(_dVars->timeScanningStarted_ms - _dVars->timeStartedLookingForFace_ms);
          int numPartialFacesSeen = 0;
          int numFullFacesSeen = 0;
          int numNamedFacesSeen = 0;
          for( const auto& faceID : _dVars->facesSeen ) {
            const auto it = _dVars->isFaceNamed.find(faceID);
            if( it != _dVars->isFaceNamed.end() && it->second ) {
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
  _dVars->persistent.lastDeactivationTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // Reset the unexpected movement mode back to what it was when this behavior activated
  auto& moveComp = GetBEI().GetMovementComponent();
  moveComp.EnableUnexpectedRotationWithoutMotors( _dVars->wasUnexpectedRotationWithoutMotorsEnabled );

  const auto& robotInfo = GetBEI().GetRobotInfo();
  // if on the charger, we're exiting to the on charger reaction, unity is going to try to cancel but too late.
  if( robotInfo.IsOnChargerContacts() )
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.CancelBecauseOnCharger","");
    SET_STATE(Cancelled);
  }

  ExternalInterface::FaceEnrollmentCompleted info;

  if( _dVars->persistent.state == State::EmotingConfusion ) {
    // interrupted while in a transient animation state. Replace with the reason for being
    // in this state
    _dVars->persistent.state = _dVars->failedState;
  }

  if( ANKI_DEVELOPER_CODE ) {
    // in unit tests, this behavior will always want to re-activate when Cancel via the delegation component,
    // unless we disable enrollment. Use a special name (one that would almost certainly never be spoken)
    if( _dVars->faceName == "Special name for unit tests to end enrollment" ) {
      SET_STATE(Success);
      DisableEnrollment();
    }
  }

  const bool wasSeeingMultipleFaces = _dVars->startedSeeingMultipleFaces_sec > 0.f;
  const bool observedUnusableFace = _dVars->observedUnusableID != Vision::UnknownFaceID && !_dVars->observedUnusableName.empty();

  // If observed ID/face are set, then it means we never found a valid, unnamed face to use
  // for enrollment, so return those in the completion message and indicate this in the result.
  // NOTE: Seeing multiple faces effectively takes precedence here.
  if(_dVars->persistent.state == State::Failed_WrongFace || (_dVars->persistent.state == State::TimedOut &&
                                                             !wasSeeingMultipleFaces &&
                                                             observedUnusableFace))
  {
    info.faceID = _dVars->observedUnusableID;
    info.name   = _dVars->observedUnusableName;
    info.result = FaceEnrollmentResult::SawWrongFace;
  }
  else
  {
    if(_dVars->saveID != Vision::UnknownFaceID)
    {
      // We just merged the enrolled ID (faceID) into saveID, so report saveID as
      // "who" was enrolled
      info.faceID = _dVars->saveID;
    }
    else
    {
      info.faceID = _dVars->faceID;
    }

    info.name   = _dVars->faceName;

    switch(_dVars->persistent.state)
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

      case State::Enrolling:
        // If deactivating while enrolling, make sure we play the interruption animation so
        // we don't leave the face with "scanning" eyes
        PlayEmergencyGetOut(AnimationTrigger::MeetVictorLookFaceInterrupt);
        info.result = FaceEnrollmentResult::Incomplete;
        break;
        
      case State::DriveOffCharger:
      case State::PutDownBlock:
      case State::WaitingInPlaceForFace:
      case State::LookingForFace:
      case State::ScanningInterrupted:
      case State::SayingName:
      case State::SayingIKnowThatName:
      case State::SayingWrongName:
      case State::SavingToRobot:
        // If we're stopping in any of these states without having timed out
        // then something else is keeping us from completing and the assumption
        // is that we'll resume and finish shortly
        info.result = FaceEnrollmentResult::Incomplete;
        break;
        
      case State::SaveFailed:
        info.result = FaceEnrollmentResult::SaveFailed;
        break;

      case State::Success:
        info.result = FaceEnrollmentResult::Success;
        break;

      case State::Failed_NameInUse:
        info.result = FaceEnrollmentResult::NameInUse;
        break;
        
      case State::Failed_NamedStorageFull:
        info.result = FaceEnrollmentResult::NamedStorageFull;
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

  int numInterruptions = _dVars->persistent.numInterruptions;

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
    const bool isNewEnrollment = Vision::UnknownFaceID != _dVars->faceID && Vision::UnknownFaceID == _dVars->saveID;
    if(info.result != FaceEnrollmentResult::Success && isNewEnrollment)
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.ErasingNewlyEnrolledFace",
                    "Erasing new face %d as a precaution because we are about to report failure result: %s",
                    _dVars->faceID, EnumToString(info.result));
      GetBEI().GetVisionComponent().EraseFace(_dVars->faceID);
    }

    if(info.result == FaceEnrollmentResult::Success)
    {
      BehaviorObjectiveAchieved(BehaviorObjective::EnrolledFaceWithName);
      GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::EnrolledFace);
    }

    // Log enrollment to DAS, with result type
    Util::sInfoF("robot.face_enrollment", {{DDATA, EnumToString(info.result)}}, "%d", info.faceID);

    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.StopInternal.BroadcastCompletion",
                  "In state:%hhu, FaceEnrollmentResult=%s",
                  _dVars->persistent.state, EnumToString(info.result));

    if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
      const auto& msgRef = info;
      ExternalInterface::FaceEnrollmentCompleted msgCopy{msgRef};
      GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame{std::move(msgCopy)} );
    }
    if( GetBEI().GetRobotInfo().HasGatewayInterface() ) {
      auto* msg = new external_interface::FaceEnrollmentCompleted{ CladProtoTypeTranslator::ToProtoEnum(info.result),
                                                                   info.faceID,
                                                                   info.name };
      GetBEI().GetRobotInfo().GetGatewayInterface()->Broadcast( ExternalMessageRouter::Wrap(msg) );
    }

    // Done (whether success or failure), so reset state for next run
    SET_STATE(NotStarted);
  } else {
    numInterruptions = ++_dVars->persistent.numInterruptions;
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
               "Completion status (Success,SawWrongFace,SawMultipleFaces,TimedOut,SaveFailed,Incomplete,Cancelled,NameInUse,NamedStorageFull,UnknownFailure)");
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
  const bool hasName = !_dVars->persistent.settings->name.empty();
  return hasName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::DisableEnrollment()
{
  _dVars->persistent.settings->name.clear();
  _dVars->persistent.didEverLeaveCharger = false;
  _dVars->persistent.lastDeactivationTime_ms = 0;
  _dVars->persistent.numInterruptions = 0;
  _dVars->persistent.wrongFaceStats.clear();
  
  // Leave "session-only" face enrollment enabled when we finish
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::ResetEnrollment()
{
  // Disable enrollment of the faceID we were enrolling
  // This also clears any partial enrollment that was in progress, which will avoid creating
  // an album entry with two people in it
  GetBEI().GetFaceWorldMutable().Enroll(Vision::UnknownFaceID);
  _dVars->lastFaceSeenTime_ms = 0;
  
  // If we are not enrolling a specific face ID, we are allowed to try again
  // with a new face, so don't hang waiting to see the one we previously picked
  if(!_dVars->enrollingSpecificID)
  {
    _dVars->faceID = Vision::UnknownFaceID;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::HasTimedOut() const
{
  const f32 currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasTimedOut = (currTime_sec > _dVars->startTime_sec + _dVars->timeout_sec);
  const bool hasSeenTooManyFacesTooLong = (_dVars->startedSeeingMultipleFaces_sec > 0.f &&
                                           (currTime_sec > _dVars->startedSeeingMultipleFaces_sec + _iConfig->tooManyFacesTimeout_sec));

  if(hasTimedOut)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.BehaviorTimedOut",
                  "TimedOut after %.1fsec in State:%s",
                  _dVars->timeout_sec, GetDebugStateName().c_str());
  }

  if(hasSeenTooManyFacesTooLong)
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HasTimedOut.TooManyFacesTooLong",
                  "Saw > %d faces for longer than %.1fsec in State:%s",
                  _iConfig->maxFacesVisible, _iConfig->tooManyFacesTimeout_sec, GetDebugStateName().c_str());
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

  if( _iConfig->driveOffChargerBehavior->WantsToBeActivated() ) {
    DelegateNow( _iConfig->driveOffChargerBehavior.get(), next);
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

  if( _iConfig->putDownBlockBehavior->WantsToBeActivated() ) {
    DelegateNow( _iConfig->putDownBlockBehavior.get(), next);
  } else {
    next();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToWaitInPlaceForFace()
{
  SET_STATE(WaitingInPlaceForFace);
  
  // Look up and wait for a few images in place before proceeding. If we were just moved or this was triggered by
  // a voice command (which turned the robot toward the sound), this gives the vision system a chance to see
  // a face from where the robot is and update FaceWorld with it, rather than immediately turning to some
  // previously-seen face in FaceWorld.
  CancelDelegates(false); // Make sure we stop tracking/scanning if necessary
  IActionRunner* action = new CompoundActionSequential({
    new MoveHeadToAngleAction(MAX_HEAD_ANGLE),
    new WaitForImagesAction(kEnrollFace_NumImagesToWaitInPlace, VisionMode::DetectingFaces),
  });
  DelegateIfInControl(action, [this](){
    TransitionToLookingForFace();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToLookingForFace()
{
  const bool playScanningGetOut = (State::Enrolling == _dVars->persistent.state);

  if( _dVars->timeStartedLookingForFace_ms == 0 ) {
    _dVars->timeStartedLookingForFace_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }
  
  // First time here, always wait in place for a face for a few frames before then getting into
  // a loop of looking for faces. Exception: SayingWrongName, since obviously a face
  // was just being observed so we can immediately turn to a new one.
  if(_dVars->persistent.state != State::WaitingInPlaceForFace
     && _dVars->persistent.state != State::LookingForFace
     && _dVars->persistent.state != State::SayingWrongName)
  {
    TransitionToWaitInPlaceForFace();
    return;
  }
  
  SET_STATE(LookingForFace);
  
  // Create an action to turn towards the "best" face in FaceWorld
  // Then wait to give the vision system a chance to see faces in that position before enrolling
  //   or looking around to find faces if there were none
  CancelDelegates(false); // Make sure we stop tracking/scanning if necessary
  IActionRunner* action = new CompoundActionSequential({
    CreateTurnTowardsFaceAction(_dVars->faceID, _dVars->saveID, playScanningGetOut),
    new WaitForImagesAction(kEnrollFace_NumImagesToWait, VisionMode::DetectingFaces),
  });

  DelegateIfInControl(action, [this]()
              {
                if(_dVars->lastFaceSeenTime_ms == 0)
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
                                  _dVars->faceID);

                    DelegateIfInControl(CreateLookAroundAction(),
                                        &BehaviorEnrollFace::TransitionToLookingForFace);
                  }
                }
                else
                {
                  // We've seen a face, so time to start enrolling it

                  // Give ourselves a little more time to finish now that we've seen a face, but
                  // don't go over the max timeout
                  _dVars->timeout_sec = std::min(kEnrollFace_TimeoutMax_sec, _dVars->timeout_sec + kEnrollFace_TimeoutExtraTime_sec);

                  PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.LookingForFace.FaceSeen",
                                "Found face %d to enroll. Timeout set to %.1fsec",
                                _dVars->faceID, _dVars->timeout_sec);

                  auto getInAnimAction = new TriggerAnimationAction(AnimationTrigger::MeetVictorGetIn);

                  IActionRunner* action = nullptr;
                  if(CanMoveTreads())
                  {
                    SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(_dVars->faceID);
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
                  if( GetBEI().GetRobotInfo().HasGatewayInterface() ) {
                    // todo: replace with generic status VIC-1423
                    auto* status = new external_interface::MeetVictorFaceScanStarted;
                    GetBEI().GetRobotInfo().GetGatewayInterface()->Broadcast( ExternalMessageRouter::Wrap(status) );
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

  _dVars->timeScanningStarted_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // Actually enable directed enrollment of the selected face in the vision system
  GetBEI().GetFaceWorldMutable().Enroll(_dVars->faceID);

  TrackFaceAction* trackAction = new TrackFaceAction(_dVars->faceID);

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

  // Play the scanning animation in parallel while we're tracking. This anim group has multiple
  // animations chosen at random. It loops forever
  auto* scanLoop = new ReselectingLoopAnimationAction{ AnimationTrigger::MeetVictorLookFace };

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

  if(_dVars->sayName)
  {
    if(_dVars->saveID == Vision::UnknownFaceID)
    {
      // If we're not being told which ID to save to, then assume this is a
      // first-time enrollment and play the bigger sequence of animations,
      // along with special music state
      // TODO: PostMusicState should take in a GameState::Music, making the cast unnecessary...
      if(_dVars->useMusic)
      {
        // NOTE: it will be up to the caller to stop this music
//        robot.GetRobotAudioClient()->PostMusicState((AudioMetaData::GameState::GenericState)AudioMetaData::GameState::Music::Minigame__Meet_Cozmo_Say_Name, false, 0);
      }

      {
        // 1. Say name once
        const auto nameQuestionStr = _dVars->faceName + "?";
        SayTextAction* sayNameAction1 = new SayTextAction(nameQuestionStr);
        sayNameAction1->SetAnimationTrigger(AnimationTrigger::MeetVictorSayName);
        finalAnimation->AddAction(sayNameAction1);
      }

      {
        // 2. Repeat name
        SayTextAction* sayNameAction2 = new SayTextAction(_dVars->faceName);
        sayNameAction2->SetAnimationTrigger(AnimationTrigger::MeetVictorSayNameAgain);
        finalAnimation->AddAction(sayNameAction2);
      }

    }
    else
    {
      // This is a re-enrollment
      SayTextAction* sayNameAction = new SayTextAction(_dVars->faceName);
      sayNameAction->SetAnimationTrigger(AnimationTrigger::MeetVictorSayName);
      finalAnimation->AddAction(sayNameAction);
    }

    // This is kinda hacky, but we could have used up a lot of our timeout time
    // during enrollment and don't want to cutoff the final animation action (which could be
    // pretty long if it's a first time enrollment), so increase our timeout at this point.
    _dVars->timeout_sec += 30.f;
  }

  // Note: even if the animation fails for some reason, we will still continue with the behavior
  DelegateIfInControl(finalAnimation, [this](ActionResult result)
  {
    if(ActionResult::SUCCESS != result)
    {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToSayingName.FinalAnimationFailed", "");
    }
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    GetBEI().GetMoodManager().TriggerEmotionEvent("EnrolledNewFace", currTime_s);
    SET_STATE(Success);
  });

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToSayingIKnowThatName()
{
  SET_STATE(SayingIKnowThatName);

  CancelDelegates(false);

  if(_dVars->sayName) {

    auto* actConfusedAnim = new TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorDuplicateName);
    DelegateIfInControl(actConfusedAnim, [this](ActionResult result) {
      
      // todo: locale
      const std::string sentence = "eye know " + _dVars->faceName;
      _iConfig->ttsBehavior->SetTextToSay( sentence );
      ANKI_VERIFY( _iConfig->ttsBehavior->WantsToBeActivated(), "BehaviorEnrollFace.TransitionToSayingIKnowThatName.NoTTS","");
      DelegateIfInControl(_iConfig->ttsBehavior.get(), [this]() {
        SET_STATE(Failed_NameInUse);
      });
    });
  } else {
    TransitionToFailedState(State::Failed_NameInUse,"Failed_NameInUse");
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToWrongFace( FaceID_t faceID, const std::string& faceName )
{
  const bool playScanningGetOut = (State::Enrolling == _dVars->persistent.state);
  
  SET_STATE(SayingWrongName);

  if(kEnrollFace_FailOnWrongFace)
  {
    _dVars->failedState = State::Failed_WrongFace;
    _dVars->observedUnusableID   = faceID;
    _dVars->observedUnusableName = faceName;
  }
  
  CancelDelegates(false);
  
  SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(faceID);
  CompoundActionParallel* action = new CompoundActionParallel({
    new TurnTowardsFaceAction(smartID, DEG_TO_RAD(30)), // Small max angle because should still be facing it
  });
  
  if(playScanningGetOut)
  {
    const u32  kNumLoops = 1;
    const bool kInterruptRunning = true;
    action->AddAction(new TriggerAnimationAction(AnimationTrigger::MeetVictorLookFaceInterrupt,
                                                 kNumLoops, kInterruptRunning,
                                                 static_cast<u8>(AnimTrackFlag::HEAD_TRACK)));
  }
  
  // Old animation sequence (too much?)
  //  const u32 kNumLoops = 1;
  //  action->AddAction(TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorGetIn));
  //  action->AddAction(TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorLookFace, kNumLoops));
  //  action->AddAction(TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorSawWrongFace));
  
  std::string text;
  
  switch(static_cast<SayWrongNameMode>(kEnrollFace_SayWrongNameMode))
  {
    case SayWrongNameMode::Off:
    {
      // Leave text empty
      break;
    }
      
    case SayWrongNameMode::Short:
      text = faceName + "!";
      break;
      
    case SayWrongNameMode::Long:
      if((faceName == _dVars->faceName)               // "wrong" name matches enrolling name
         && (_dVars->saveID == Vision::UnknownFaceID) // this is a re-enrollment
         && ANKI_VERIFY( faceID != _dVars->saveID,    // saveID should not also match "wrong" ID
                        "BehaviorEnrollFace.TransitionToWrongFace.NotWrongFace",
                        "'Wrong' face matches enrolling name ('%s') and saveID (%d): not possible?",
                        faceName.c_str(), _dVars->saveID))
      {
        // Weird special case if we ever support enrolling multiple people with the same name.
        // Avoid saying something confusing like "You're Bob, not Bob" if we are re-enrolloing
        // a different "Bob" with a different saveID
        text = "already know you!";
      } else {
        text = /* you're */ "your " + faceName + ", not " + _dVars->faceName + "!";
      }
      break;
  }

  DelegateIfInControl(action, [this,text=std::move(text),faceName,faceID](ActionResult result) {
    
    // Mark that we've said this name, so we don't do it again
    auto it = _dVars->persistent.wrongFaceStats.find(faceName);
    if(ANKI_VERIFY(it != _dVars->persistent.wrongFaceStats.end(), "BehaviorEnrollFace.TransitionToWrongFace.MissingStats",
                   "ID:%d Name:%s", faceID, Util::HidePersonallyIdentifiableInfo(faceName.c_str())))
    {
      it->second.nameSaid = true;
    }
    
    if(text.empty())
    {
      TransitionToLookingForFace();
    }
    else
    {
      // todo: locale
      _iConfig->ttsBehavior->SetTextToSay( text );
      ANKI_VERIFY( _iConfig->ttsBehavior->WantsToBeActivated(), "BehaviorEnrollFace.TransitionToWrongFace.NoTTS","");
      DelegateIfInControl(_iConfig->ttsBehavior.get(), [this]() {
        if(kEnrollFace_FailOnWrongFace)
        {
          _dVars->persistent.state = State::Failed_WrongFace;
          SetDebugStateName("Failed_WrongFace");
        }
        else
        {
          // Continue looking for faces
          TransitionToLookingForFace();
        }
      });
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::TransitionToFailedState( State state, const std::string& stateName )
{
  SET_STATE(EmotingConfusion);
  _dVars->failedState = state;

  CancelDelegates(false);

  auto* action = new TriggerLiftSafeAnimationAction(AnimationTrigger::MeetVictorConfusion);

  DelegateIfInControl(action, [this, state, stateName](ActionResult result) {
    if( ActionResult::SUCCESS != result ) {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToFailedState.FinalAnimationFailed", "");
    }
    _dVars->persistent.state = state;
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

  // TODO: Does this need to be done asynchronously?
  const Result saveResult = GetBEI().GetVisionComponent().SaveFaceAlbum();
  if(RESULT_OK == saveResult)
  {
    TransitionToSayingName();
  }
  else
  {
    // If save failed, robot will not remember the name on a restart, so this is a failed enrollment
    SET_STATE(SaveFailed);
  }
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
    const s32 kNumLoops = 1;
    const bool kInterruptRunning = true;
    liftAndTurnTowardsAction->AddAction(new TriggerAnimationAction(AnimationTrigger::MeetVictorLookFaceInterrupt,
                                                                   kNumLoops, kInterruptRunning,
                                                                   static_cast<u8>(AnimTrackFlag::HEAD_TRACK)));
    
  }

  if(!CanMoveTreads())
  {
    // If being held in the air, don't try to turn, so just return the parallel
    // compound action as it is now
    return liftAndTurnTowardsAction;
  }

  auto const& faceWorld = GetBEI().GetFaceWorld();
  
  SmartFaceID smartID;
  if(GetBEI().GetFaceWorld().HasAnyFaces())
  {
    if( (faceID != Vision::UnknownFaceID) && (nullptr != faceWorld.GetFace(faceID)))
    {
      // Try to look at the specified face
      PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.TurningTowardsFaceID",
                    "Turning towards faceID=%d (saveID=%d)",
                    faceID, saveID);
      
      smartID = faceWorld.GetSmartFaceID(faceID);
    }
    else if( (saveID != Vision::UnknownFaceID) && (nullptr != faceWorld.GetFace(saveID)) )
    {
      // If saveID was specified, check first to see if it's present in FaceWorld and turn towards
      // it if so(since that's who we are re-enrolling)
      smartID = faceWorld.GetSmartFaceID(saveID);
    }
    else
    {
      // Select the "best" face according to selection criteria
      const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
      smartID = faceSelection.GetBestFaceToUse( _iConfig->faceSelectionCriteria );
      
      // If nothing better is available, the face selector could return a named face, which we don't want
      auto face = faceWorld.GetFace(smartID);
      if(!ANKI_VERIFY(nullptr != face, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.NullBestFace",
                      "SmartFaceID %s returned as best but not in FaceWorld", smartID.GetDebugStr().c_str())
         || face->HasName())
      {
        smartID.Reset();
      }
    }
  }
  
  IActionRunner* turnAction = nullptr;
  if(smartID.IsValid())
  {
    turnAction = new TurnTowardsFaceAction(smartID, kEnrollFace_MaxTurnTowardsFaceAngle_rad);
  }
  else
  {
    // Couldn't find face in face world, try turning towards last face pose
    PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.CreateTurnTowardsFaceAction.NullFace",
                  "No face found to turn towards. FaceID=%d. SaveID=%d. Turning towards last face pose.",
                  faceID, saveID);
    
    // No face found to look towards: fallback on looking at last face pose
    turnAction = new TurnTowardsLastFacePoseAction(kEnrollFace_MaxTurnTowardsFaceAngle_rad);
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
                                        -_dVars->lastRelBodyAngle.ToDouble());
  const Radians relBodyAngle = newAngle -_dVars->lastRelBodyAngle;
  _dVars->lastRelBodyAngle = newAngle;

  CompoundActionSequential* compoundAction = new CompoundActionSequential();

  if(CanMoveTreads())
  {
    compoundAction->AddAction(new PanAndTiltAction(relBodyAngle, absHeadAngle, false, true));

    // Also back up a little if we haven't gone too far back already
    if(_dVars->totalBackup_mm <= kEnrollFace_MaxTotalBackup_mm)
    {
      const f32 backupSpeed_mmps = 100.f;
      const f32 backupDist_mm = GetRNG().RandDblInRange(kEnrollFace_MinBackup_mm, kEnrollFace_MaxBackup_mm);
      _dVars->totalBackup_mm += backupDist_mm;
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
  _dVars->faceID = newFace->GetID();
  _dVars->lastFaceSeenTime_ms = newFace->GetTimeStamp();

  _dVars->observedUnusableName.clear();
  _dVars->observedUnusableID = Vision::UnknownFaceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsSeeingTooManyFaces(FaceWorld& faceWorld, const RobotTimeStamp_t lastImgTime)
{
  // Check if we've also seen too many within a recent time window
  const TimeStamp_t multipleFaceTimeWindow_ms = Util::SecToMilliSec(_iConfig->tooManyFacesRecentTime_sec);
  const RobotTimeStamp_t recentTime = (lastImgTime > multipleFaceTimeWindow_ms ?
                                      lastImgTime - multipleFaceTimeWindow_ms :
                                      0); // Avoid unsigned math rollover

  auto recentlySeenFaceIDs = faceWorld.GetFaceIDs(recentTime);

  for( const auto& faceID : recentlySeenFaceIDs ) {
    const Vision::TrackedFace* face = faceWorld.GetFace(faceID);
    // only save info on the face if it is known this tick, but don't remove saved faces
    if( nullptr != face ) {
      _dVars->facesSeen.insert( faceID );
      _dVars->isFaceNamed[faceID] = (faceID > 0) && face->HasName();
    }
  }


  const bool hasRecentlySeenTooManyFaces = recentlySeenFaceIDs.size() > _iConfig->maxFacesVisible;
  if(hasRecentlySeenTooManyFaces)
  {
    if(_dVars->startedSeeingMultipleFaces_sec == 0.f)
    {
      // We just started seeing too many faces
      _dVars->startedSeeingMultipleFaces_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

      // Disable enrollment while seeing too many faces
      faceWorld.Enroll(Vision::UnknownFaceID);

      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StartedSeeingTooMany",
                     "Disabling enrollment (if enabled) at t=%.1f", _dVars->startedSeeingMultipleFaces_sec);
    }
    return true;
  }
  else
  {
    if(_dVars->startedSeeingMultipleFaces_sec > 0.f)
    {
      PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.StoppedSeeingTooMany",
                     "Stopped seeing too many at t=%.1f", _dVars->startedSeeingMultipleFaces_sec);

      // We are not seeing too many faces any more (and haven't recently), so reset this to zero
      _dVars->startedSeeingMultipleFaces_sec = 0.f;

      if(_dVars->faceID != Vision::UnknownFaceID)
      {
        // Re-enable enrollment of whatever we were enrolling before we started seeing too many faces
        faceWorld.Enroll(_dVars->faceID);

        PRINT_CH_DEBUG(kLogChannelName, "BehaviorEnrollFace.IsSeeingTooManyFaces.RestartEnrollment",
                       "Re-enabling enrollment of FaceID:%d", _dVars->faceID);

      }
    }
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEnrollFace::IsSeeingWrongFace(FaceID_t& wrongFaceID, std::string& wrongName) const
{
  // Find any entry whose name we haven't already said and either whose ID changed or it's been
  // seen while looking for faces too many times
  auto it = std::find_if( _dVars->persistent.wrongFaceStats.begin(), _dVars->persistent.wrongFaceStats.end(),
                         [](const auto& mapEntry) {
                           const DynamicVariables::WrongFaceInfo& info = mapEntry.second;
                           const bool countTooHigh = (info.count >= kEnrollFace_TicksForKnownNameBeforeFail);
                           return !info.nameSaid && (info.idChanged || countTooHigh);
                         });
  
  if(it != _dVars->persistent.wrongFaceStats.end())
  {
    // Return the ID and name
    wrongFaceID = it->second.faceID;
    wrongName   = it->first;
    return true;
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEnrollFace::UpdateFaceToEnroll()
{
  const FaceWorld& faceWorld = GetBEI().GetFaceWorld();
  auto& robotInfo = GetBEI().GetRobotInfo();

  const RobotTimeStamp_t lastImgTime = robotInfo.GetLastImageTimeStamp();

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
  auto observedFaceIDs = faceWorld.GetFaceIDs(lastImgTime);

  const bool enrollmentIDisSet    = (_dVars->faceID != Vision::UnknownFaceID);
  const bool sawCurrentEnrollFace = (enrollmentIDisSet && observedFaceIDs.count(_dVars->faceID));

  if(sawCurrentEnrollFace)
  {
    // If we saw the face we're currently enrolling, there's nothing to do other than
    // update it's last seen time
    const Face* enrollFace = GetBEI().GetFaceWorld().GetFace(_dVars->faceID);
    UpdateFaceIDandTime(enrollFace);
  }
  else
  {
    // Didn't see current face (or don't have one yet). Look at others and see if
    // we want to switch to any of them.
    for(auto faceID : observedFaceIDs)
    {
      // We just checked that _faceID *wasn't* seen!
      DEV_ASSERT(faceID != _dVars->faceID, "BehaviorEnrollFace.UpdateFaceToEnroll.UnexpectedFaceID");

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
                            "FaceID %d came back null", faceID);
        continue;
      }

      // Record the last person we saw (that we weren't already enrolling), in
      // case we fail and need to message that the reason why was that we we were
      // seeing this named face. These get cleared if we end up using this observed
      // face for enrollment.
      _dVars->observedUnusableID   = faceID;
      _dVars->observedUnusableName = newFace->GetName();

      // We can only switch to this observed faceID if it is unnamed, _unless_
      // it matches the saveID.
      // - for new enrollments we can't enroll an already-named face (that's a re-enrollment, by definition)
      // - for re-enrollment, a face with a name must be the one we are re-enrolling
      // - if the name matches the face ID, then the faceID matches too and we wouldn't even
      //   be considering this observation because there's no ID change
      const bool canUseObservedFace = !newFace->HasName() || (faceID == _dVars->saveID);

      if(canUseObservedFace)
      {
        if(enrollmentIDisSet)
        {
          // Face ID is already set but we didn't see it and instead we're seeing a face
          // with a different ID. See if it matches the pose of the one we were already enrolling.

          auto currentFace = GetBEI().GetFaceWorld().GetFace(_dVars->faceID);

          if(nullptr != currentFace && nullptr != newFace &&
             newFace->GetHeadPose().IsSameAs(currentFace->GetHeadPose(),
                                             kEnrollFace_UpdateFacePositionThreshold_mm,
                                             DEG_TO_RAD(kEnrollFace_UpdateFaceAngleThreshold_deg)))
          {
            PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.UpdateFaceToEnroll.UpdatingFaceIDbyPose",
                          "Was enrolling ID=%d, changing to unnamed ID=%d based on pose (saveID=%d)",
                          _dVars->faceID, faceID, _dVars->saveID);

            UpdateFaceIDandTime(newFace);
          }

        }
        else
        {
          // We don't have a face ID set yet. Use this one, since it passed all the earlier checks
          PRINT_CH_INFO(kLogChannelName,
                        "BehaviorEnrollFace.UpdateFaceToEnroll.SettingInitialFaceID",
                        "Set face ID to unnamed face %d (saveID=%d)", faceID, _dVars->saveID);

          UpdateFaceIDandTime(newFace);
        }
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName,
                      "BehaviorEnrollFace.UpdateFaceToEnroll.IgnoringObservedFace",
                      "Refusing to enroll '%s' face %d, with current faceID=%d and saveID=%d",
                      !newFace->HasName() ? "<unnamed>" : Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()),
                      faceID, _dVars->faceID, _dVars->saveID);
        
        if(newFace->HasName())
        {
          // Update the number of times we've seen this named face
          auto it = _dVars->persistent.wrongFaceStats.find(newFace->GetName());
          if( it == _dVars->persistent.wrongFaceStats.end() ) {
            // New entry
            _dVars->persistent.wrongFaceStats.emplace(newFace->GetName(), DynamicVariables::WrongFaceInfo(faceID));
          } else {
            // Increment existing
            ++it->second.count;
          }
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
      if(msg.oldID == _dVars->faceID)
      {
        const Vision::TrackedFace* newFace = GetBEI().GetFaceWorld().GetFace(msg.newID);
        if(msg.newID != _dVars->saveID &&
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
                        Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()), _dVars->saveID);

          // Mark any existing entry for this face as having had its ID updated, or create new entry if needed
          auto it = _dVars->persistent.wrongFaceStats.find(newFace->GetName());
          if(it != _dVars->persistent.wrongFaceStats.end())
          {
            ++it->second.count;
            it->second.idChanged = true;
          }
          else
          {
            _dVars->persistent.wrongFaceStats.emplace(newFace->GetName(),
                                                      DynamicVariables::WrongFaceInfo(msg.newID, true));
          }
        }
        else
        {
          PRINT_CH_INFO(kLogChannelName, "BehaviorEnrollFace.HandleRobotChangedObservedFaceID.UpdatingFaceID",
                        "Was enrolling ID=%d, changing to ID=%d",
                        _dVars->faceID, msg.newID);
          _dVars->faceID = msg.newID;
        }
      }

      if(msg.oldID == _dVars->saveID)
      {
        // I don't think this should happen: we should never update a saveID because it
        // should be named, meaning we should never merge into it
        PRINT_NAMED_ERROR("BehaviorEnrollFace.HandleRobotChangedObservedFaceID.SaveIDChanged",
                          "Was saving to ID=%d, which apparently changed to %d. Should not happen. Will abort.",
                          _dVars->saveID, msg.newID);
        TransitionToFailedState(State::Failed_UnknownReason,"Failed_UnknownReason");
      }
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorEnrollFace.AlwaysHandle.UnexpectedEngineToGameTag",
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

      *(_dVars->persistent.settings) = msg;
      _dVars->persistent.requestedRescan = true;
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
