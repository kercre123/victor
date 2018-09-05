/**
 * File: behaviorInteractWithFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "InteractWithFaces" behavior, which tracks/interacts with faces if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorInteractWithFaces.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/trackedFace.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define CONSOLE_GROUP "Behavior.InteractWithFaces"

namespace Anki {
namespace Vector {

namespace {

// how far forward to check and ideally drive
CONSOLE_VAR_RANGED(f32, kInteractWithFaces_DriveForwardIdealDist_mm, CONSOLE_GROUP, 40.0f, 0.0f, 200.0f);

// how far forward to move in case the check fails
CONSOLE_VAR_RANGED(f32, kInteractWithFaces_DriveForwardMinDist_mm, CONSOLE_GROUP, -15.0f, -100.0f, 100.0f);

// if true, do a glance down before the memory map check (only valid if we are doing the check)
// TODO:(bn) could check memory map for Unknown, and only glance down in that case
CONSOLE_VAR(bool, kInteractWithFaces_DoGlanceDown, CONSOLE_GROUP, false);

// if false, always drive the "ideal" distance without checking anything. If true, check memory map to
// determine which distance to drive
CONSOLE_VAR(bool, kInteractWithFaces_DoMemoryMapCheckForDriveForward, CONSOLE_GROUP, true);

CONSOLE_VAR(bool, kInteractWithFaces_VizMemoryMapCheck, CONSOLE_GROUP, false);

CONSOLE_VAR_RANGED(f32, kInteractWithFaces_DriveForwardSpeed_mmps, CONSOLE_GROUP, 40.0f, 0.0f, 200.0f);

// Minimum angles to turn during tracking to keep the robot moving and looking alive
CONSOLE_VAR_RANGED(f32, kInteractWithFaces_MinTrackingPanAngle_deg,  CONSOLE_GROUP, 4.0f, 0.0f, 30.0f);
CONSOLE_VAR_RANGED(f32, kInteractWithFaces_MinTrackingTiltAngle_deg, CONSOLE_GROUP, 4.0f, 0.0f, 30.0f);

// If we are doing the memory map check, these are the types which will prevent us from driving the ideal
// distance
constexpr MemoryMapTypes::FullContentArray typesToBlockDriving =
{
  {MemoryMapTypes::EContentType::Unknown               , false},
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , true },
  {MemoryMapTypes::EContentType::ObstacleProx          , true },
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
  {MemoryMapTypes::EContentType::Cliff                 , true },
  {MemoryMapTypes::EContentType::InterestingEdge       , true },
  {MemoryMapTypes::EContentType::NotInterestingEdge    , true }
};
static_assert(MemoryMapTypes::IsSequentialArray(typesToBlockDriving),
  "This array does not define all types once and only once.");
  
const char* const kMinTimeToTrackFaceKeyLowerBoundKey = "minTimeToTrackFaceLowerBound_s";
const char* const kMinTimeToTrackFaceKeyUpperBoundKey = "minTimeToTrackFaceUpperBound_s";
const char* const kMaxTimeToTrackFaceKeyLowerBoundKey = "maxTimeToTrackFaceLowerBound_s";
const char* const kMaxTimeToTrackFaceKeyUpperBoundKey = "maxTimeToTrackFaceUpperBound_s";
const char* const kNoEyeContactTimeoutKey = "noEyeContactTimeout_s";
const char* const kEyeContactWithinLastKey = "eyeContactWithinLast_ms";
const char* const kTrackingTimeoutKey = "trackingTimeout_s";
const char* const kClampSmallAnglesKey = "clampSmallAngles";
const char* const kMinClampPeriodKey = "minClampPeriod_s";
const char* const kMaxClampPeriodKey = "maxClampPeriod_s";
const char* const kChanceSayName = "chanceSayName";

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInteractWithFaces::InstanceConfig::InstanceConfig()
{
  minTimeToTrackFaceLowerBound_s = 0.0f;
  minTimeToTrackFaceUpperBound_s = 0.0f;
  maxTimeToTrackFaceLowerBound_s = 0.0f;
  maxTimeToTrackFaceUpperBound_s = 0.0f;
  minClampPeriod_s               = 0.0f;
  maxClampPeriod_s               = 0.0f;
  noEyeContactTimeout_s          = 0.0f;
  eyeContactWithinLast_ms        = 0;
  trackingTimeout_s              = 0.0f;
  clampSmallAngles               = false;
  chanceSayName                  = 0.1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInteractWithFaces::DynamicVariables::DynamicVariables()
{
  lastImageTimestampWhileRunning = 0;
  trackFaceUntilTime_s           = -1.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInteractWithFaces::BehaviorInteractWithFaces(const Json::Value& config)
: ICozmoBehavior(config)
{
  LoadConfig(config);

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
   kMinTimeToTrackFaceKeyLowerBoundKey,
   kMinTimeToTrackFaceKeyUpperBoundKey,
   kMaxTimeToTrackFaceKeyLowerBoundKey,
   kMaxTimeToTrackFaceKeyUpperBoundKey,
   kEyeContactWithinLastKey,
   kNoEyeContactTimeoutKey,
   kTrackingTimeoutKey,
   kClampSmallAnglesKey,
   kMinClampPeriodKey,
   kMaxClampPeriodKey,
   kChanceSayName,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = "BehaviorInteractWithFaces.BehaviorInteractWithFaces.LoadConfig";

  _iConfig.minTimeToTrackFaceLowerBound_s = ParseFloat(config, kMinTimeToTrackFaceKeyLowerBoundKey, debugName);
  _iConfig.minTimeToTrackFaceUpperBound_s = ParseFloat(config, kMinTimeToTrackFaceKeyUpperBoundKey, debugName);
  _iConfig.maxTimeToTrackFaceLowerBound_s = ParseFloat(config, kMaxTimeToTrackFaceKeyLowerBoundKey, debugName);
  _iConfig.maxTimeToTrackFaceUpperBound_s = ParseFloat(config, kMaxTimeToTrackFaceKeyUpperBoundKey, debugName);
  _iConfig.noEyeContactTimeout_s          = ParseFloat(config, kNoEyeContactTimeoutKey, debugName);
  _iConfig.eyeContactWithinLast_ms        = ParseInt32(config, kEyeContactWithinLastKey, debugName);
  _iConfig.trackingTimeout_s              = ParseFloat(config, kTrackingTimeoutKey, debugName);
  _iConfig.chanceSayName                  = ParseFloat(config, kChanceSayName, debugName);

  if( ! ANKI_VERIFY(_iConfig.maxTimeToTrackFaceLowerBound_s >= _iConfig.minTimeToTrackFaceUpperBound_s,
                    "BehaviorInteractWithFaces.LoadConfig.InvalidTrackingTime",
                    "%s: minTrackTimeUpperBound = %f, maxTrackTimeLowerBound = %f",
                    GetDebugLabel().c_str(),
                    _iConfig.minTimeToTrackFaceUpperBound_s,
                    _iConfig.maxTimeToTrackFaceLowerBound_s) ) {
    _iConfig.maxTimeToTrackFaceLowerBound_s = _iConfig.minTimeToTrackFaceUpperBound_s;
  }

  if( ! ANKI_VERIFY(_iConfig.minTimeToTrackFaceUpperBound_s >= _iConfig.minTimeToTrackFaceLowerBound_s,
                    "BehaviorInteractWithFaces.LoadConfig.InvalidTrackingTime",
                    "%s: minTrackTimeUpperBound = %f, minTrackTimeLowerBound = %f",
                    GetDebugLabel().c_str(),
                    _iConfig.minTimeToTrackFaceUpperBound_s,
                    _iConfig.minTimeToTrackFaceLowerBound_s) ) {
    _iConfig.minTimeToTrackFaceUpperBound_s = _iConfig.minTimeToTrackFaceLowerBound_s;
  }

  if( ! ANKI_VERIFY(_iConfig.maxTimeToTrackFaceUpperBound_s >= _iConfig.maxTimeToTrackFaceLowerBound_s,
                    "BehaviorInteractWithFaces.LoadConfig.InvalidTrackingTime",
                    "%s: maxTrackTimeUpperBound = %f, maxTrackTimeLowerBound = %f",
                    GetDebugLabel().c_str(),
                    _iConfig.maxTimeToTrackFaceUpperBound_s,
                    _iConfig.maxTimeToTrackFaceLowerBound_s) ) {
    _iConfig.maxTimeToTrackFaceUpperBound_s = _iConfig.minTimeToTrackFaceLowerBound_s;
  }

  _iConfig.clampSmallAngles = ParseBool(config, kClampSmallAnglesKey, debugName);
  if( _iConfig.clampSmallAngles ) {
    _iConfig.minClampPeriod_s = ParseFloat(config, kMinClampPeriodKey, debugName);
    _iConfig.maxClampPeriod_s = ParseFloat(config, kMinClampPeriodKey, debugName);

    if( ! ANKI_VERIFY(_iConfig.maxClampPeriod_s >= _iConfig.minClampPeriod_s,
                      "BehaviorInteractWithFaces.LoadConfig.InvalidClampPeriod",
                      "%s: minPeriod = %f, maxPeriod = %f",
                      GetDebugLabel().c_str(),
                      _iConfig.minClampPeriod_s,
                      _iConfig.maxClampPeriod_s) ) {
      _iConfig.maxClampPeriod_s = _iConfig.minClampPeriod_s;
    }

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::OnBehaviorActivated()
{
  // reset the time to stop tracking (in the tracking state)
  _dVars.trackFaceUntilTime_s = -1.0f;

  if( _dVars.targetFace.IsValid() ) {
    TransitionToInitialReaction();
  }
  else {
    PRINT_NAMED_WARNING("BehaviorInteractWithFaces.Init.NoValidTarget",
                        "Decided to run, but don't have valid target when Init is called. This shouldn't happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if( _dVars.trackFaceUntilTime_s >= 0.0f ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( currTime_s >= _dVars.trackFaceUntilTime_s ) {
      BehaviorObjectiveAchieved(BehaviorObjective::InteractedWithFace);
      CancelDelegates();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInteractWithFaces::WantsToBeActivatedBehavior() const
{
  _dVars.targetFace.Reset();
  SelectFaceToTrack();

  return _dVars.targetFace.IsValid();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::OnBehaviorDeactivated()
{
  _dVars.lastImageTimestampWhileRunning = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInteractWithFaces::CanDriveIdealDistanceForward()
{
  if( kInteractWithFaces_DoMemoryMapCheckForDriveForward && 
      GetBEI().HasMapComponent()) {
    const auto& robotInfo = GetBEI().GetRobotInfo();

    const auto memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
    
    DEV_ASSERT(nullptr != memoryMap, "BehaviorInteractWithFaces.CanDriveIdealDistanceForward.NeedMemoryMap");

    const Vec3f& fromRobot = robotInfo.GetPose().GetTranslation();

    const Vec3f ray{kInteractWithFaces_DriveForwardIdealDist_mm, 0.0f, 0.0f};
    const Vec3f toGoal = robotInfo.GetPose() * ray;
    
    const bool hasCollision = memoryMap->HasCollisionWithTypes({{Point2f{fromRobot}, Point2f{toGoal}}}, typesToBlockDriving);

    if( kInteractWithFaces_VizMemoryMapCheck ) {
      const char* vizID = "BehaviorInteractWithFaces.MemMapCheck";
      const float zOffset_mm = 15.0f;
      robotInfo.GetContext()->GetVizManager()->EraseSegments(vizID);
      robotInfo.GetContext()->GetVizManager()->DrawSegment(vizID,
                                                       fromRobot, toGoal,
                                                       hasCollision ? Anki::NamedColors::YELLOW
                                                                    : Anki::NamedColors::BLUE,
                                                       false,
                                                       zOffset_mm);
    }

    const bool canDriveIdealDist = !hasCollision;
    return canDriveIdealDist;
  }
  else {
    // always drive ideal distance
    return true;
  }
}

#pragma mark -
#pragma mark State Machine

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToInitialReaction()
{
  DEBUG_SET_STATE(VerifyFace);

  CompoundActionSequential* action = new CompoundActionSequential();

  {

    const bool sayName = GetRNG().RandDbl() < _iConfig.chanceSayName;
    TurnTowardsFaceAction* turnAndAnimateAction = new TurnTowardsFaceAction(_dVars.targetFace, M_PI_F, sayName);
    // TODO VIC-6435 uncomment this once we've removed turn from animation
    //turnAndAnimateAction->SetSayNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialNamed);
    turnAndAnimateAction->SetNoNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialUnnamed);
    turnAndAnimateAction->SetRequireFaceConfirmation(true);
    action->AddAction(turnAndAnimateAction);
  }
  
  DelegateIfInControl(action, [this](ActionResult ret ) {
      if( ret == ActionResult::SUCCESS ) {
        TransitionToGlancingDown();
      }
      else {
        // one possible cause of failure is that the face id we tried to track wasn't there (but another face
        // was). So, see if there is a new "best face", and if so, track that one. This will only run if a new
        // face is observed.

        if(GetBEI().HasMoodManager()){
          // increase frustration to avoid loops
          auto& moodManager = GetBEI().GetMoodManager();
          moodManager.TriggerEmotionEvent("InteractWithFaceRetry",
                                          MoodManager::GetCurrentTimeInSeconds());
        }
        
        _dVars.lastImageTimestampWhileRunning =  GetBEI().GetRobotInfo().GetLastImageTimeStamp();
        
        SmartFaceID oldTargetFace = _dVars.targetFace;
        SelectFaceToTrack();
        if(_dVars.targetFace != oldTargetFace) {
          // only retry a max of one time to avoid loops
          PRINT_CH_INFO("Behaviors","BehaviorInteractWithFaces.InitialReactionFailed.TryAgain",
                        "tracking face %s failed, but will try again with face %s",
                        oldTargetFace.GetDebugStr().c_str(),
                        _dVars.targetFace.GetDebugStr().c_str());

          TransitionToInitialReaction();
        }
        else {
          PRINT_CH_INFO("Behaviors","BehaviorInteractWithFaces.InitialReactionFailed",
                        "compound action failed with result '%s', not retrying",
                        ActionResultToString(ret));
        }
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToGlancingDown()
{
  DEBUG_SET_STATE(GlancingDown);

  if( kInteractWithFaces_DoGlanceDown && kInteractWithFaces_DoMemoryMapCheckForDriveForward ) {
    // TODO:(bn) get a better measurement for this and put it in cozmo config
    const float kLowHeadAngle_rads = DEG_TO_RAD(-10.0f);
    DelegateIfInControl(new MoveHeadToAngleAction(kLowHeadAngle_rads),
                &BehaviorInteractWithFaces::TransitionToDrivingForward);
  }
  else {
    TransitionToDrivingForward();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToDrivingForward()
{
  DEBUG_SET_STATE(DrivingForward);
  
  // check if we should do the long or short distance
  const bool doLongDrive = CanDriveIdealDistanceForward();
  const float distToDrive_mm = doLongDrive ?
    kInteractWithFaces_DriveForwardIdealDist_mm :
    kInteractWithFaces_DriveForwardMinDist_mm;

  // drive straight while keeping the head tracking the (players) face
  CompoundActionParallel* action = new CompoundActionParallel();

  // the head tracking action normally loops forever, so set up the drive action first, tell it to emit
  // completion signals, then pass it's tag in to the tracking action so the tracking action can stop itself
  // when the driving action finishes

  u32 driveActionTag = 0;
  {
    // don't play driving animations (to avoid sounds which don't make sense here)
    // TODO:(bn) custom driving animations for this action?
    DriveStraightAction* driveAction = new DriveStraightAction(distToDrive_mm,
                                                               kInteractWithFaces_DriveForwardSpeed_mmps,
                                                               false);
    const bool ignoreFailure = false;
    const bool emitCompletionSignal = true;
    action->AddAction( driveAction, ignoreFailure, emitCompletionSignal );
    driveActionTag = driveAction->GetTag();
  }

  {
    TrackFaceAction* trackWithHeadAction = new TrackFaceAction(_dVars.targetFace);
    trackWithHeadAction->SetMode(ITrackAction::Mode::HeadOnly);
    trackWithHeadAction->StopTrackingWhenOtherActionCompleted( driveActionTag );
    trackWithHeadAction->SetTiltTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingPanAngle_deg));
    trackWithHeadAction->SetPanTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingTiltAngle_deg));
    trackWithHeadAction->SetClampSmallAnglesToTolerances(_iConfig.clampSmallAngles);
    trackWithHeadAction->SetClampSmallAnglesPeriod(_iConfig.minClampPeriod_s, _iConfig.maxClampPeriod_s);

    action->AddAction( trackWithHeadAction );
  }
  
  // TODO:(bn) alternate driving animations?
  DelegateIfInControl(action, &BehaviorInteractWithFaces::TransitionToTrackingFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToTrackingFace()
{
  DEBUG_SET_STATE(TrackingFace);

  const float randomMaxTimeToTrack_s = Util::numeric_cast<float>(
    GetRNG().RandDblInRange(_iConfig.maxTimeToTrackFaceLowerBound_s, _iConfig.maxTimeToTrackFaceUpperBound_s));
  PRINT_CH_INFO("Behaviors", "BehaviorInteractWithFaces.TransitionToTrackingFace.MaxTrackTime",
    "will track for at most %f seconds", randomMaxTimeToTrack_s);
  _dVars.trackFaceUntilTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() +
    randomMaxTimeToTrack_s;

  const float randomMinTimeToTrack_s = Util::numeric_cast<float>(
    GetRNG().RandDblInRange(_iConfig.minTimeToTrackFaceLowerBound_s, _iConfig.minTimeToTrackFaceUpperBound_s));
  PRINT_CH_INFO("Behaviors", "BehaviorInteractWithFaces.TransitionToTrackingFace.MinTrackTime",
    "will track for at least %f seconds", randomMinTimeToTrack_s);


  CompoundActionParallel* action = new CompoundActionParallel();

  {
    TrackFaceAction* trackAction = new TrackFaceAction(_dVars.targetFace);
    trackAction->SetTiltTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingPanAngle_deg));
    trackAction->SetPanTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingTiltAngle_deg));
    trackAction->SetClampSmallAnglesToTolerances(_iConfig.clampSmallAngles);
    trackAction->SetClampSmallAnglesPeriod(_iConfig.minClampPeriod_s, _iConfig.maxClampPeriod_s);
    trackAction->SetUpdateTimeout(_iConfig.trackingTimeout_s);
    trackAction->SetEyeContactContinueCriteria(randomMinTimeToTrack_s, _iConfig.noEyeContactTimeout_s,
                                               _iConfig.eyeContactWithinLast_ms);
    action->AddAction(trackAction);
  }
  
  // loop animation forever to keep the eyes moving
  action->AddAction(new TriggerAnimationAction(AnimationTrigger::InteractWithFaceTrackingIdle, 0));
  action->SetShouldEndWhenFirstActionCompletes(true);
  
  EngineTimeStamp_t eyeContactStart_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  DelegateIfInControl(action, [this,eyeContactStart_ms]() {
    // send info on eye contact duration
    TimeStamp_t duration_ms = TimeStamp_t(BaseStationTimer::getInstance()->GetCurrentTimeStamp() - eyeContactStart_ms);
    DASMSG(behavior_interactwithfaces_trackingended, "behavior.interactwithfaces.trackingended", "Face tracking ended");
    DASMSG_SET(i1, duration_ms, "Duration (ms) of tracking, which is related to the duration of eye contact");
    DASMSG_SEND();
    
    TransitionToTriggerEmotionEvent();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToTriggerEmotionEvent()
{
  DEBUG_SET_STATE(TriggerEmotionEvent);

  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    const Vision::TrackedFace* face = GetBEI().GetFaceWorld().GetFace( _dVars.targetFace );
    
    if( nullptr != face && face->HasName() ) {
      moodManager.TriggerEmotionEvent("InteractWithNamedFace", MoodManager::GetCurrentTimeInSeconds());
    }
    else {
      moodManager.TriggerEmotionEvent("InteractWithUnnamedFace", MoodManager::GetCurrentTimeInSeconds());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::SelectFaceToTrack() const
{  
  const bool considerTrackingOnlyFaces = false;
  auto smartFaceIDs = GetBEI().GetFaceWorld().GetSmartFaceIDs(_dVars.lastImageTimestampWhileRunning,
                                                              considerTrackingOnlyFaces);
  
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  FaceSelectionComponent::FaceSelectionFactorMap criteriaMap;
  criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::UnnamedFace, 1000));
  criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, 1));
  criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 3));
  _dVars.targetFace = faceSelection.GetBestFaceToUse(criteriaMap, smartFaceIDs);
}


} // namespace Vector
} // namespace Anki
