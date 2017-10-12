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
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/trackedFace.h"

#include "util/console/consoleInterface.h"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define CONSOLE_GROUP "Behavior.InteractWithFaces"

namespace Anki {
namespace Cozmo {

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
  {MemoryMapTypes::EContentType::ObstacleCube          , true },
  {MemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {MemoryMapTypes::EContentType::ObstacleCharger       , true },
  {MemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {MemoryMapTypes::EContentType::ObstacleProx          , true },
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
  {MemoryMapTypes::EContentType::Cliff                 , true },
  {MemoryMapTypes::EContentType::InterestingEdge       , true },
  {MemoryMapTypes::EContentType::NotInterestingEdge    , true }
};
static_assert(MemoryMapTypes::IsSequentialArray(typesToBlockDriving),
  "This array does not define all types once and only once.");

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInteractWithFaces::BehaviorInteractWithFaces(const Json::Value& config)
: ICozmoBehavior(config)
{
  LoadConfig(config["params"]);

  SubscribeToTags({ EngineToGameTag::RobotChangedObservedFaceID });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = "BehaviorInteractWithFaces.BehaviorInteractWithFaces.LoadConfig";

  _configParams.minTimeToTrackFace_s = ParseFloat(config, "minTimeToTrackFace_s", debugName);
  _configParams.maxTimeToTrackFace_s = ParseFloat(config, "maxTimeToTrackFace_s", debugName);

  if( ! ANKI_VERIFY(_configParams.maxTimeToTrackFace_s >= _configParams.minTimeToTrackFace_s,
                    "BehaviorInteractWithFaces.LoadConfig.InvalidTrackingTime",
                    "%s: minTrackTime = %f, maxTrackTime = %f",
                    GetIDStr().c_str(),
                    _configParams.minTimeToTrackFace_s,
                    _configParams.maxTimeToTrackFace_s) ) {
    _configParams.maxTimeToTrackFace_s = _configParams.minTimeToTrackFace_s;
  }

  _configParams.clampSmallAngles = ParseBool(config, "clampSmallAngles", debugName);
  if( _configParams.clampSmallAngles ) {
    _configParams.minClampPeriod_s = ParseFloat(config, "minClampPeriod_s", debugName);
    _configParams.maxClampPeriod_s = ParseFloat(config, "maxClampPeriod_s", debugName);

    if( ! ANKI_VERIFY(_configParams.maxClampPeriod_s >= _configParams.minClampPeriod_s,
                      "BehaviorInteractWithFaces.LoadConfig.InvalidClampPeriod",
                      "%s: minPeriod = %f, maxPeriod = %f",
                      GetIDStr().c_str(),
                      _configParams.minClampPeriod_s,
                      _configParams.maxClampPeriod_s) ) {
      _configParams.maxClampPeriod_s = _configParams.minClampPeriod_s;
    }

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorInteractWithFaces::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // reset the time to stop tracking (in the tracking state)
  _trackFaceUntilTime_s = -1.0f;

  if( _targetFace != Vision::UnknownFaceID ) {
    TransitionToInitialReaction(behaviorExternalInterface);
    return RESULT_OK;
  }
  else {
    PRINT_NAMED_WARNING("BehaviorInteractWithFaces.Init.NoValidTarget",
                        "Decided to run, but don't have valid target when Init is called. This shouldn't happen");
    return RESULT_FAIL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorInteractWithFaces::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _trackFaceUntilTime_s >= 0.0f ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( currTime_s >= _trackFaceUntilTime_s ) {
      BehaviorObjectiveAchieved(BehaviorObjective::InteractedWithFace);
      StopActing();
      
      if(behaviorExternalInterface.HasNeedsManager()){
        auto& needsManager = behaviorExternalInterface.GetNeedsManager();
        needsManager.RegisterNeedsActionCompleted(NeedsActionId::SeeFace);
      }
    }
  }
  
  return BaseClass::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInteractWithFaces::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  _targetFace = Vision::UnknownFaceID;
  SelectFaceToTrack(behaviorExternalInterface);

  return _targetFace != Vision::UnknownFaceID;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  _lastImageTimestampWhileRunning = robot.GetLastImageTimeStamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInteractWithFaces::CanDriveIdealDistanceForward(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( kInteractWithFaces_DoMemoryMapCheckForDriveForward ) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();

    const INavMap* memoryMap = robot.GetMapComponent().GetCurrentMemoryMap();
    
    DEV_ASSERT(nullptr != memoryMap, "BehaviorInteractWithFaces.CanDriveIdealDistanceForward.NeedMemoryMap");

    const Vec3f& fromRobot = robot.GetPose().GetTranslation();

    const Vec3f ray{kInteractWithFaces_DriveForwardIdealDist_mm, 0.0f, 0.0f};
    const Vec3f toGoal = robot.GetPose() * ray;
    
    const bool hasCollision = memoryMap->HasCollisionRayWithTypes(fromRobot, toGoal, typesToBlockDriving);

    if( kInteractWithFaces_VizMemoryMapCheck ) {
      const char* vizID = "BehaviorInteractWithFaces.MemMapCheck";
      const float zOffset_mm = 15.0f;
      robot.GetContext()->GetVizManager()->EraseSegments(vizID);
      robot.GetContext()->GetVizManager()->DrawSegment(vizID,
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
void BehaviorInteractWithFaces::TransitionToInitialReaction(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(VerifyFace);

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  {
    TurnTowardsFaceAction* turnAndAnimateAction = new TurnTowardsFaceAction(robot, _targetFace, M_PI_F, true);
    turnAndAnimateAction->SetSayNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialNamed);
    turnAndAnimateAction->SetNoNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialUnnamed);
    turnAndAnimateAction->SetRequireFaceConfirmation(true);
    action->AddAction(turnAndAnimateAction);
  }
  
  DelegateIfInControl(action, [this, &behaviorExternalInterface](ActionResult ret ) {
      if( ret == ActionResult::SUCCESS ) {
        TransitionToGlancingDown(behaviorExternalInterface);
      }
      else {
        // one possible cause of failure is that the face id we tried to track wasn't there (but another face
        // was). So, see if there is a new "best face", and if so, track that one. This will only run if a new
        // face is observed.

        if(behaviorExternalInterface.HasMoodManager()){
          // increase frustration to avoid loops
          auto& moodManager = behaviorExternalInterface.GetMoodManager();
          moodManager.TriggerEmotionEvent("InteractWithFaceRetry",
                                          MoodManager::GetCurrentTimeInSeconds());
        }
        
        {
          // DEPRECATED - Grabbing robot to support current cozmo code, but this should
          // be removed
          const Robot& robot = behaviorExternalInterface.GetRobot();
          _lastImageTimestampWhileRunning = robot.GetLastImageTimeStamp();
        }
        FaceID_t oldTargetFace = _targetFace;
        SelectFaceToTrack(behaviorExternalInterface);
        if( _targetFace != oldTargetFace ) {
          // only retry a max of one time to avoid loops
          PRINT_CH_INFO("Behaviors","BehaviorInteractWithFaces.InitialReactionFailed.TryAgain",
                        "tracking face %d failed, but will try again with face %d",
                        oldTargetFace,
                        _targetFace);

          TransitionToInitialReaction(behaviorExternalInterface);
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
void BehaviorInteractWithFaces::TransitionToGlancingDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(GlancingDown);

  if( kInteractWithFaces_DoGlanceDown && kInteractWithFaces_DoMemoryMapCheckForDriveForward ) {
    // TODO:(bn) get a better measurement for this and put it in cozmo config
    const float kLowHeadAngle_rads = DEG_TO_RAD(-10.0f);
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new MoveHeadToAngleAction(robot, kLowHeadAngle_rads),
                &BehaviorInteractWithFaces::TransitionToDrivingForward);
  }
  else {
    TransitionToDrivingForward(behaviorExternalInterface);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToDrivingForward(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DrivingForward);
  
  // check if we should do the long or short distance
  const bool doLongDrive = CanDriveIdealDistanceForward(behaviorExternalInterface);
  const float distToDrive_mm = doLongDrive ?
    kInteractWithFaces_DriveForwardIdealDist_mm :
    kInteractWithFaces_DriveForwardMinDist_mm;

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // drive straight while keeping the head tracking the (players) face
  CompoundActionParallel* action = new CompoundActionParallel(robot);

  // the head tracking action normally loops forever, so set up the drive action first, tell it to emit
  // completion signals, then pass it's tag in to the tracking action so the tracking action can stop itself
  // when the driving action finishes

  u32 driveActionTag = 0;
  {
    // don't play driving animations (to avoid sounds which don't make sense here)
    // TODO:(bn) custom driving animations for this action?
    DriveStraightAction* driveAction = new DriveStraightAction(robot,
                                                               distToDrive_mm,
                                                               kInteractWithFaces_DriveForwardSpeed_mmps,
                                                               false);
    const bool ignoreFailure = false;
    const bool emitCompletionSignal = true;
    action->AddAction( driveAction, ignoreFailure, emitCompletionSignal );
    driveActionTag = driveAction->GetTag();
  }

  {
    TrackFaceAction* trackWithHeadAction = new TrackFaceAction(robot, _targetFace);
    trackWithHeadAction->SetMode(ITrackAction::Mode::HeadOnly);
    trackWithHeadAction->StopTrackingWhenOtherActionCompleted( driveActionTag );
    trackWithHeadAction->SetTiltTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingPanAngle_deg));
    trackWithHeadAction->SetPanTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingTiltAngle_deg));
    trackWithHeadAction->SetClampSmallAnglesToTolerances(_configParams.clampSmallAngles);
    trackWithHeadAction->SetClampSmallAnglesPeriod(_configParams.minClampPeriod_s, _configParams.maxClampPeriod_s);

    action->AddAction( trackWithHeadAction );
  }
  
  // TODO:(bn) alternate driving animations?
  DelegateIfInControl(action, &BehaviorInteractWithFaces::TransitionToTrackingFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToTrackingFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(TrackingFace);

  const float randomTimeToTrack_s = Util::numeric_cast<float>(
    GetRNG().RandDblInRange(_configParams.minTimeToTrackFace_s, _configParams.maxTimeToTrackFace_s));
  PRINT_CH_INFO("Behaviors", "BehaviorInteractWithFaces.TrackTime", "will track for %f seconds", randomTimeToTrack_s);
  _trackFaceUntilTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + randomTimeToTrack_s;

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  CompoundActionParallel* action = new CompoundActionParallel(robot);

  {
    TrackFaceAction* trackAction = new TrackFaceAction(robot, _targetFace);
    trackAction->SetTiltTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingPanAngle_deg));
    trackAction->SetPanTolerance(DEG_TO_RAD(kInteractWithFaces_MinTrackingTiltAngle_deg));
    trackAction->SetClampSmallAnglesToTolerances(_configParams.clampSmallAngles);
    trackAction->SetClampSmallAnglesPeriod(_configParams.minClampPeriod_s, _configParams.maxClampPeriod_s);
    action->AddAction(trackAction);
  }
  
  // loop animation forever to keep the eyes moving
  action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::InteractWithFaceTrackingIdle, 0));
  
  DelegateIfInControl(action, &BehaviorInteractWithFaces::TransitionToTriggerEmotionEvent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::TransitionToTriggerEmotionEvent(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(TriggerEmotionEvent);

  if(behaviorExternalInterface.HasMoodManager()){
    auto& moodManager = behaviorExternalInterface.GetMoodManager();
    const Vision::TrackedFace* face = behaviorExternalInterface.GetFaceWorld().GetFace( _targetFace );
    
    if( nullptr != face && face->HasName() ) {
      moodManager.TriggerEmotionEvent("InteractWithNamedFace", MoodManager::GetCurrentTimeInSeconds());
    }
    else {
      moodManager.TriggerEmotionEvent("InteractWithUnnamedFace", MoodManager::GetCurrentTimeInSeconds());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::SelectFaceToTrack(BehaviorExternalInterface& behaviorExternalInterface) const
{  
  const bool considerTrackingOnlyFaces = false;
  std::set< FaceID_t > faces = behaviorExternalInterface.GetFaceWorld().GetFaceIDsObservedSince(_lastImageTimestampWhileRunning,
                                                                                 considerTrackingOnlyFaces);

  const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  const bool preferName = true;
  _targetFace = whiteboard.GetBestFaceToTrack(faces, preferName);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInteractWithFaces::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  if( event.GetData().GetTag() == EngineToGameTag::RobotChangedObservedFaceID )
  {
    auto const& msg = event.GetData().Get_RobotChangedObservedFaceID();
    if( msg.oldID == _targetFace ) {
      _targetFace = msg.newID;
    }
  }
}


} // namespace Cozmo
} // namespace Anki
