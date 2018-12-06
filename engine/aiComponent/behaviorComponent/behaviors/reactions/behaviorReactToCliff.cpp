/**
 * File: behaviorReactToCliff.cpp
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/drivePathAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/components/movementComponent.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/navMap/mapComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/proxMessages.h"

#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#include <limits>

#define CONSOLE_GROUP "Behavior.ReactToCliff"

namespace Anki {
namespace Vector {

using namespace ExternalInterface;

namespace {
  const char* kCliffBackupDistKey = "cliffBackupDistance_mm";
  const char* kCliffBackupSpeedKey = "cliffBackupSpeed_mmps";
  const char* kEventFlagTimeoutKey = "eventFlagTimeout_ms";
  
  // If the robot is at a steep pitch, it's possible it's been put down
  // purposefully on a slope, so this behavior won't activate until enough time
  // has passed with the robot on its treads in order to give `ReactToPlacedOnSlope`
  // time to activate and run.
  const f32 kMinPitchToCheckForIncline_rad = DEG_TO_RAD(10.f);
  
  // When the robot is at a steep pitch, there is an additional requirement that the
  // robot should consider itself "OnTreads" for at least this number of ms to
  // prevent interrupting the `ReactToPlacedOnSlope` behavior.
  const u16 kMinTimeSinceOffTreads_ms = 1000;
  
  // When the behavior wants to delegate to a behavior that asks for help because
  // it thinks the robot is stuck on an edge, we should wait for a certain
  // period of time to make sure that the robot is stationary (not picked up)
  // before delegating to the StuckOnEdge behavior, or else that behavior will
  // not activate or cancel itself if the robot is teetering on an edge.
  const u16 kMinTimeToValidateStuckOnEdge_ms = 200;

  // If the value of the cliff when it started stopping is
  // this much less than the value when it stopped, then
  // the cliff is considered suspicious (i.e. something like
  // carpet) and the reaction is aborted.
  // In general you'd expect the start value to be _higher_
  // that the stop value if it's true cliff, but we
  // use some margin here to account for sensor noise.
  static const u16   kSuspiciousCliffValDiff = 20;

  // minimum number of images with edge detection activated
  const int kNumImagesToWaitForEdges = 1;

  // rate of turning the robot while searching for cliffs with vision
  const f32 kBodyTurnSpeedForCliffSearch_degPerSec = 120.0f;

  // If this many RobotStopped messages are received
  // while activated, then just give up and go to StuckOnEdge/AskForHelp.
  // It's probably too dangerous to keep trying anything
  CONSOLE_VAR(uint32_t, kMaxNumRobotStopsBeforeGivingUp, CONSOLE_GROUP, 5);
  
  // If the behavior encounters this many failures with the animations or actions
  // while activated, then just give up and go to StuckOnEdge/AskForHelp.
  CONSOLE_VAR(uint32_t, kMaxNumCliffReactAttemptsBeforeGivingUp, CONSOLE_GROUP, 2);

  // whether this experimental feature whereby the robot uses vision
  // to extend known cliffs via edge detection is active
  CONSOLE_VAR(bool, kEnableVisualCliffExtension, CONSOLE_GROUP, true);

  CONSOLE_VAR(float, kMinViewingDistanceToCliff_mm, CONSOLE_GROUP, 80.0f);

  CONSOLE_VAR(float,  kProceduralReactionArcRadius_mm, CONSOLE_GROUP, 200);
  CONSOLE_VAR(float,  kProceduralReactionArcAngle_rad, CONSOLE_GROUP, 0.40);
  CONSOLE_VAR(float,  kProceduralReactionArcSpeed_mmps, CONSOLE_GROUP, -250);
  CONSOLE_VAR(float,  kProceduralReactionArcAccel_mmps2, CONSOLE_GROUP, 500);
  CONSOLE_VAR(float,  kProceduralReactionArcDecel_mmps2, CONSOLE_GROUP, 500);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCliffBackupDistKey,
    kCliffBackupSpeedKey,
    kEventFlagTimeoutKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::InstanceConfig::InstanceConfig()
{
  stuckOnEdgeBehavior = nullptr;
  askForHelpBehavior = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  cliffBackupDist_mm = JsonTools::ParseFloat(config, kCliffBackupDistKey, debugName);
  // Verify that backup distance is valid.
  ANKI_VERIFY(Util::IsFltGTZero(cliffBackupDist_mm), (debugName + ".InvalidCliffBackupDistance").c_str(),
              "Value should be greater than 0.0 (not %.2f).", cliffBackupDist_mm);
  
  cliffBackupSpeed_mmps = JsonTools::ParseFloat(config, kCliffBackupSpeedKey, debugName);
  // Verify that backup speed is valid.
  ANKI_VERIFY(Util::IsFltGTZero(cliffBackupSpeed_mmps), (debugName + ".InvalidCliffBackupSpeed").c_str(),
              "Value should be greater than 0.0 (not %.2f).", cliffBackupSpeed_mmps);
  
  eventFlagTimeout_ms = JsonTools::ParseUInt32(config, kEventFlagTimeoutKey, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::DynamicVariables::DynamicVariables()
{
  quitReaction = false;
  gotStop = false;
  wantsToBeActivated = false;
  hasTargetCliff = false;

  cliffPose.ClearParent();
  cliffPose = Pose3d(); // identity
  
  const auto kInitVal = std::numeric_limits<u16>::max();
  std::fill(persistent.cliffValsAtStart.begin(), persistent.cliffValsAtStart.end(), kInitVal);
  persistent.numStops = 0;
  persistent.numCliffReactAttempts = 0;
  persistent.putDownOnCliff = false;
  persistent.lastInScopeTime_ms = 0.f;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::BehaviorReactToCliff(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig = InstanceConfig(config, debugName);
  
  SubscribeToTags({{
    EngineToGameTag::RobotStopped,
    EngineToGameTag::RobotOffTreadsStateChanged,
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.stuckOnEdgeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(StuckOnEdge));
  _iConfig.askForHelpBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(AskForHelp));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCliff::WantsToBeActivatedBehavior() const
{
  if (_dVars.gotStop || _dVars.wantsToBeActivated || _dVars.persistent.putDownOnCliff)
  {
    if( GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads ) {
      const Radians& pitch =  GetBEI().GetRobotInfo().GetPitchAngle();
      if( pitch >= DEG_TO_RAD(kMinPitchToCheckForIncline_rad) ) {
        const EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        const EngineTimeStamp_t lastChangedTime_ms = GetBEI().GetRobotInfo().GetOffTreadsStateLastChangedTime_ms();
        return (currTime_ms - lastChangedTime_ms >= kMinTimeSinceOffTreads_ms);
      }
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorActivated()
{
  DEBUG_SET_STATE(Activating);
  // reset dvars
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());
  } 
    
  auto& robotInfo = GetBEI().GetRobotInfo();

  // Wait function for determining if the cliff is suspicious
  auto waitForStopLambda = [this, &robotInfo](Robot& robot) {
    if ( robotInfo.GetMoveComponent().AreWheelsMoving() ) {
      return false;
    }

    const auto& cliffComp = robotInfo.GetCliffSensorComponent();
    const auto& cliffData = cliffComp.GetCliffDataRaw();

    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.CliffValsAtEnd", 
                  "%u %u %u %u (%x)", 
                  cliffData[0], cliffData[1], cliffData[2], cliffData[3], cliffComp.GetCliffDetectedFlags());

    for (int i=0; i<CliffSensorComponent::kNumCliffSensors; ++i) {  
      if (cliffComp.IsCliffDetected((CliffSensor)(i))) {
        if (_dVars.persistent.cliffValsAtStart[i] + kSuspiciousCliffValDiff < cliffData[i]) {
          // There was a sufficiently large increase in cliff value since cliff was 
          /// first detected so assuming it was a false cliff and aborting reaction
          PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.QuittingDueToSuspiciousCliff", 
                        "index: %d, startVal: %u, currVal: %u",
                        i, _dVars.persistent.cliffValsAtStart[i], cliffData[i]);
          _dVars.quitReaction = true;    
        }
      }
    }

    // compute the pose of the detected cliff, 
    // and cache it for determining lookat positions
    if(cliffComp.GetCliffPoseRelativeToRobot(cliffComp.GetCliffDetectedFlags(), _dVars.cliffPose)) {
      _dVars.cliffPose.SetParent(GetBEI().GetRobotInfo().GetPose());
      if (_dVars.cliffPose.GetWithRespectTo(GetBEI().GetRobotInfo().GetWorldOrigin(), _dVars.cliffPose)) {
        _dVars.hasTargetCliff = true;
      } else {
        PRINT_NAMED_WARNING("BehaviorReactToCliff.OnBehaviorActivated.OriginMismatch",
                            "cliffPose and WorldOrigin do not share the same origin!");
      }
    } else {
      PRINT_NAMED_WARNING("BehaviorReactToCliff.OnBehaviorActivated.NoPoseForCliffFlags",
                          "flags=%hhu", cliffComp.GetCliffDetectedFlags());
    }

    return true;
  };

  // skip the "huh" animation if in severe energy or repair
  WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(waitForStopLambda);
  DelegateIfInControl(waitForStopAction, &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToStuckOnEdge()
{
  DEBUG_SET_STATE(StuckOnEdge);
  if ( _iConfig.stuckOnEdgeBehavior->WantsToBeActivated() ) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    DASMSG(behavior_cliff_stuck_on_edge, "behavior.cliff_stuck_on_edge", "The robot is stuck on the edge of a surface");
    DASMSG_SET(i1, robotInfo.GetCliffSensorComponent().GetCliffDetectedFlags(), "Cliff detected flags");
    DASMSG_SEND();
    
    DelegateIfInControl(_iConfig.stuckOnEdgeBehavior.get());
  } else {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToStuckOnEdge.DoesNotWantToBeActivated",
                  "Behavior %s does not want to be activated, re-starting cliff reaction",
                  _iConfig.stuckOnEdgeBehavior->GetDebugLabel().c_str());
    // We should ALWAYS be able to delegate to the AskForHelp behavior,
    // i.e. no activation conditions should block this delegation
    if (ANKI_VERIFY(_iConfig.askForHelpBehavior->WantsToBeActivated(),
                    "BehaviorReactToCliff.TransitionToStuckOnEdge.DoesNotWantToBeActivated",
                    "Behavior %s does not want to be activated!!",
                    _iConfig.askForHelpBehavior->GetDebugLabel().c_str()))
    {
      DelegateIfInControl(_iConfig.askForHelpBehavior.get());
    }
  }
  // Behavior will cancel itself if it reaches this point
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToPlayingCliffReaction()
{
  DEBUG_SET_STATE(PlayingCliffReaction);

  if(_dVars.quitReaction) {
    return;
  }
  
  // send DAS event for activation. don't send it if quitReaction is false, because the user likely didn't notice
  {
    DASMSG(behavior_cliffreaction, "behavior.cliffreaction", "The robot reacted to a cliff");
    DASMSG_SET(i1, _dVars.persistent.cliffValsAtStart[0], "Sensor value 1 (front left)");
    DASMSG_SET(i2, _dVars.persistent.cliffValsAtStart[1], "Sensor value 2 (front right)");
    DASMSG_SET(i3, _dVars.persistent.cliffValsAtStart[2], "Sensor value 3 (back left)");
    DASMSG_SET(i4, _dVars.persistent.cliffValsAtStart[3], "Sensor value 4 (back right)");
    DASMSG_SEND();
  }

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::ReactedToCliff);
  
  // Have we tried executing too many cliff reaction animations/actions for this
  // activation of the behavior? We must either be in a "cliffy" area, or somehow stuck in
  // some orientation that StuckOnEdge is not detecting. Just go to StuckOnEdge to be safe.
  if (_dVars.persistent.numCliffReactAttempts > kMaxNumCliffReactAttemptsBeforeGivingUp) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToPlayingCliffReaction.TooManyCliffReactionAttempts", "");
    TransitionToStuckOnEdge();
    return;
  }
  
  if(ShouldStreamline()){
    // If we skip over the animation, we still count the recovery backup as the cliff reaction attempt.
    ++_dVars.persistent.numCliffReactAttempts;
    TransitionToRecoveryBackup();
  } else {
    Anki::Util::sInfo("robot.cliff_detected", {}, "");

    auto cliffDetectedFlags = GetBEI().GetRobotInfo().GetCliffSensorComponent().GetCliffDetectedFlags();
    if (cliffDetectedFlags == 0) {
      // For some reason no cliffs are detected. Just leave the reaction.
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToPlayingCliffReaction.NoCliffs", "");
      CancelSelf();
      return;
    }

    // Did we get too many RobotStopped messages for this
    // activation of the behavior? Must be in a very "cliffy" area.
    // Just go to StuckOnEdge to be safe.
    if (_dVars.persistent.numStops > kMaxNumRobotStopsBeforeGivingUp) {
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToPlayingCliffReaction.TooManyRobotStops", "");
      TransitionToStuckOnEdge();
      return;
    }

    // Get the pre-react action/animation to play
    auto action = GetCliffReactAction(cliffDetectedFlags);
    if (action == nullptr) {
      // No action was returned because the detected cliffs represent 
      // a precarious situation so just delegate to StuckOnEdge.
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToPlayingCliffReaction.StuckOnEdge", 
                       "%x", 
                       cliffDetectedFlags);
      TransitionToStuckOnEdge();
    } else {
      ++_dVars.persistent.numCliffReactAttempts;
      DelegateIfInControl(action, [this](const ActionResult& res){
        if ( !GetBEI().GetRobotInfo().GetCliffSensorComponent().IsCliffDetected() ) {
          // The action reported success or partially succeeded moving the robot enough to get it away from the cliff.
          TransitionToFaceAndBackAwayCliff();
        } else {
          // Cliff reaction failed for some reason, something might be preventing the robot from moving away from
          // the cliff (e.g. the robot was teetering and unsure of whether it's been picked up, or the treads
          // slipped and got the robot stuck on the edge of a cliff).
          TransitionToWaitForNoMotion();
        }
      });
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToWaitForNoMotion()
{
  DEBUG_SET_STATE(WaitForNoMotion);
  
  // Wait function to allow the robot to stop teetering on an edge if it is stuck.
  auto waitForStationaryLambda = [this](Robot& robot) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    const bool areWheelsMoving = robotInfo.GetMoveComponent().AreWheelsMoving();
    const auto timeSinceLastTreadStateChange_ms =
    BaseStationTimer::getInstance()->GetCurrentTimeStamp() - robotInfo.GetOffTreadsStateLastChangedTime_ms();
    if ( areWheelsMoving ||
        robotInfo.IsPickedUp() ||
        timeSinceLastTreadStateChange_ms < kMinTimeToValidateStuckOnEdge_ms) {
      PRINT_PERIODIC_CH_INFO(5, "Behaviors", "BehaviorReactToCliff.WaitForNoMotion",
                             "Wheels moving? %d, Picked up? %d, Last time tread's state changed? %u ms",
                             areWheelsMoving, robotInfo.IsPickedUp(), static_cast<u32>(timeSinceLastTreadStateChange_ms));
      return false;
    }
    // Once we've stabilized, double check that we're still detecting cliffs underneath the robot.
    const auto& cliffComp = robotInfo.GetCliffSensorComponent();
    if (!cliffComp.IsCliffDetected()) {
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.WaitForNoMotion.QuittingDueToNoCliff", "");
      _dVars.quitReaction = true;
    }
    return true;
  };
  
  WaitForLambdaAction* waitForStationaryAction = new WaitForLambdaAction(waitForStationaryLambda);
  // NOTE: If, after waiting for all motion to stop, the robot is actually stuck on an edge, TransitionToPlayingCliffReaction
  // will catch this and call TransitionToStuckOnEdge (see above).
  DelegateIfInControl(waitForStationaryAction, &BehaviorReactToCliff::TransitionToPlayingCliffReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToRecoveryBackup()
{
  DEBUG_SET_STATE(RecoveryBackup);
  auto& robotInfo = GetBEI().GetRobotInfo();

  // if the animation doesn't drive us backwards enough, do it manually
  if( robotInfo.GetCliffSensorComponent().IsCliffDetected() ) {
    
    // Determine whether to backup or move forward based on triggered sensor
    f32 direction = 1.f;
    if (robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FL) ||
        robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FR) ) {
      direction = -1.f;
    }

    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToRecoveryBackup.DoingExtraRecoveryMotion", "");
    IActionRunner* backupAction = new DriveStraightAction(direction * _iConfig.cliffBackupDist_mm, _iConfig.cliffBackupSpeed_mmps);
    BehaviorSimpleCallback callback = [this](void)->void{
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToRecoveryBackup.ExtraRecoveryMotionComplete", "");
      auto& cliffComponent = GetBEI().GetRobotInfo().GetCliffSensorComponent();
      if ( cliffComponent.IsCliffDetected() ) {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToRecoveryBackup.StillStuckOnEdge", 
                          "%x", 
                          cliffComponent.GetCliffDetectedFlags());
        TransitionToStuckOnEdge();
      } else if (_dVars.persistent.putDownOnCliff) {
        TransitionToHeadCalibration();
      } else {
        TransitionToVisualExtendCliffs();
      }
    };
    DelegateIfInControl(backupAction, callback);
  } else if (_dVars.persistent.putDownOnCliff) {
    TransitionToHeadCalibration();
  } else {
    TransitionToVisualExtendCliffs();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToHeadCalibration()
{
  DEBUG_SET_STATE(CalibratingHead);
  // The `putDownOnCliff` flag is what triggers the calling of this method.
  // To avoid causing a loop, reset it here, since we're about to calibrate the head motor.
  _dVars.persistent.putDownOnCliff = false;
  // Force all motors to recalibrate since it's possible that Vector may have been put down too aggressively,
  // resulting in gear slippage for a motor, or the user might have forced one of the motors into a different
  // position while in the air or while sensors were disabled.
  DelegateIfInControl(new CalibrateMotorAction(true, true,
                                               EnumToString(MotorCalibrationReason::BehaviorReactToCliff)),
                      &BehaviorReactToCliff::TransitionToVisualExtendCliffs);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Pose3d BehaviorReactToCliff::GetCliffPoseToLookAt() const
{
  Pose3d cliffLookAtPose;
  if(_dVars.hasTargetCliff) {
    bool result = _dVars.cliffPose.GetWithRespectTo(GetBEI().GetRobotInfo().GetWorldOrigin(), cliffLookAtPose);
    if(result) {
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.GetCliffLookAtPose.CliffAt", 
                      "x=%4.2f y=%4.2f",
                      _dVars.cliffPose.GetTranslation().x(),
                      _dVars.cliffPose.GetTranslation().y());
    } else {
      PRINT_NAMED_WARNING("BehaviorReactToCliff.GetCliffLookAtPose.CliffPoseNotInSameTreeAsCurrentWorldOrigin","");
    }
  } else {
    // no previously set target cliff pose to look at
    // instead look at arbitrary position in front
    PRINT_NAMED_WARNING("BehaviorReactToCliff.GetCliffLookAtPose.CliffDefaultPoseAssumed","");
    cliffLookAtPose = Pose3d(0.f, Z_AXIS_3D(), {60.f, 0.f, 0.f});
    cliffLookAtPose.SetParent(GetBEI().GetRobotInfo().GetPose());
    cliffLookAtPose.GetWithRespectTo(GetBEI().GetRobotInfo().GetWorldOrigin(), cliffLookAtPose);
  }

  return cliffLookAtPose;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToVisualExtendCliffs()
{
  if(!kEnableVisualCliffExtension) {
    return;
  }
  
  DEBUG_SET_STATE(VisuallyExtendingCliffs);
  Pose3d cliffLookAtPose = GetCliffPoseToLookAt();

  
  CompoundActionSequential* compoundAction = new CompoundActionSequential();

  compoundAction->AddAction(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK)); // move lift to be out of the FOV

  // sometimes the animation will have us slightly off from the pose
  auto turnAction = new TurnTowardsPoseAction(cliffLookAtPose);
  turnAction->SetMaxPanSpeed(DEG_TO_RAD(kBodyTurnSpeedForCliffSearch_degPerSec));
  compoundAction->AddAction(turnAction);

  // if we're too close to the cliff, we need to backup to view it better
  float dist = ComputeDistanceBetween(cliffLookAtPose.GetTranslation(), GetBEI().GetRobotInfo().GetPose().GetTranslation());
  if(dist < kMinViewingDistanceToCliff_mm) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToVisualCliffExtend.FurtherBackupNeeded", "%6.6fmm", dist);
    // note: we will always be facing the cliff, so the backup direction is set at this point
    compoundAction->AddAction(new DriveStraightAction(-(kMinViewingDistanceToCliff_mm-dist), _iConfig.cliffBackupSpeed_mmps));
    // secondary look-at action to ensure we're facing the cliff point
    // note: this will correct any offset introduced by the backup action
    auto turnAction2 = new TurnTowardsPoseAction(cliffLookAtPose);
    turnAction2->SetMaxPanSpeed(DEG_TO_RAD(kBodyTurnSpeedForCliffSearch_degPerSec));
    compoundAction->AddAction(turnAction2);
  }
  compoundAction->AddAction(new WaitForImagesAction(kNumImagesToWaitForEdges, VisionMode::DetectingOverheadEdges));
  
  DelegateIfInControl(compoundAction, []() {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.TransitionToVisualCliffExtend.ObservationFinished", "");
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorDeactivated()
{
  DEBUG_SET_STATE(Inactive);
  // reset dvars
  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorLeftActivatableScope() 
{
  _dVars.persistent.lastInScopeTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.stuckOnEdgeBehavior.get());
  delegates.insert(_iConfig.askForHelpBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::BehaviorUpdate()
{
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();
  
  if(!IsActivated()){
    const auto& currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    const auto& latestCliffStopTime_ms = cliffComp.GetLatestStopDueToCliffTime_ms();
    const auto& lastPutDownOnCliffTime_ms = cliffComp.GetLatestPutDownOnCliffTime_ms();
    
    const auto& timeSinceLastStop_ms = currentTime_ms - latestCliffStopTime_ms;
    const auto& timeSinceLastPutDownOnCliff_ms = currentTime_ms - lastPutDownOnCliffTime_ms;
    
    if (_dVars.gotStop) {
      if (timeSinceLastStop_ms > _iConfig.eventFlagTimeout_ms) {
        _dVars.gotStop = false;
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.IgnoreLastStopEvent", "");
      }
    } else {
      // Check for stops due to a cliff while the behavior was out of scope,
      // but still ignore events that occurred past a certain timeout horizon
      if ( (latestCliffStopTime_ms > _dVars.persistent.lastInScopeTime_ms) &&
           ( (timeSinceLastStop_ms < _iConfig.eventFlagTimeout_ms) ||
           // The robot drove and stopped on a cliff while the behavior was not in scope,
           // and was not allowed to update until a much later time, so we double-check to
           // see if we are still stopped over a cliff (i.e. the robot hasn't moved since)
           cliffComp.IsCliffDetected() ) ) {
        _dVars.gotStop = true;
        ++_dVars.persistent.numStops;
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.PossibleStopEventWhileOutOfScope", "");
        // Record triggered cliff sensor value(s) and compare to what they are when wheels
        // stop moving. If the values are higher, the cliff is suspicious and we should quit.
        // NOTE(GB): Due to the delay between the latest cliff sensor data and the RobotStopped
        // event, the raw data values at the time of retrieval may not register a cliff being
        // detected, since the latest cliff data can arrive up to ~30 ms after the message arrives.
        const auto& cliffData = cliffComp.GetCliffDataRawAtLastStop();
        std::copy(cliffData.begin(), cliffData.end(), _dVars.persistent.cliffValsAtStart.begin());
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.CliffValsAtLastStopOutOfScope",
                      "%u %u %u %u",
                      _dVars.persistent.cliffValsAtStart[0],
                      _dVars.persistent.cliffValsAtStart[1],
                      _dVars.persistent.cliffValsAtStart[2],
                      _dVars.persistent.cliffValsAtStart[3]);
      }
    }
    
    if (_dVars.persistent.putDownOnCliff) {
      if (timeSinceLastPutDownOnCliff_ms > _iConfig.eventFlagTimeout_ms) {
        _dVars.persistent.putDownOnCliff = false;
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.IgnoreLastPossiblePutDownOnCliffEvent", "");
      }
    } else {
      // Check for events where the robot was put down on a cliff while the behavior was out of scope,
      // but double check that at the time that the behavior re-entered the activation scope,
      // that the robot is still on a cliff.
      if ( (lastPutDownOnCliffTime_ms > _dVars.persistent.lastInScopeTime_ms) &&
           cliffComp.IsCliffDetected() ) {
        _dVars.persistent.putDownOnCliff = true;
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.PutDownOnCliffEventWhileOutOfScope", "");
      }
    }
    // Set wantsToBeActivated to effectively give the activation conditions
    // an extra tick to be evaluated.
    _dVars.wantsToBeActivated = _dVars.gotStop || _dVars.persistent.putDownOnCliff;
    _dVars.gotStop = false;
    return;
  }

  // TODO: This exit on unexpected movement is probably good to have, 
  // but it appears the cliff reactions cause unexpected movement to 
  // trigger falsely, so commenting out until unexpected movement
  // is modified to have fewer false positives.
  //  
  // // Delegate to StuckOnEdge if unexpected motion detected while
  // // cliff is still detected since it means treads are spinning
  // const bool unexpectedMovement = GetBEI().GetMovementComponent().IsUnexpectedMovementDetected();
  // const bool cliffDetected = GetBEI().GetRobotInfo().GetCliffSensorComponent().IsCliffDetected();
  // if (unexpectedMovement && cliffDetected) {
  //   PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.StuckOnEdge", "");
  //   _iConfig.stuckOnEdgeBehavior->WantsToBeActivated();
  //   DelegateNow(_iConfig.stuckOnEdgeBehavior.get());
  // }

  // Cancel if picked up
  const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();
  // Often, when the robot gets too close to a curved edge, the robot can teeter and trigger a false
  // positive for pick-up detection. To counter this we wait for more than half of the cliff sensors
  // to also report that they are detecting "cliffs", due to the robot getting picked up. Otherwise,
  // we wait until the next engine tick to check all conditions

  u8 cliffsDetected = CliffSensorComponent::kNumCliffSensors;
  for (int i=0; i<CliffSensorComponent::kNumCliffSensors; ++i) {
    if (!cliffComp.IsCliffDetected((CliffSensor)(i))) {
      // Robot is reporting that it has been picked up, but not all cliff sensors have reported
      // seing cliffs yet, which would be expected if the robot is teetering on an edge for
      // some reason, so it might lead to a false-positive detection of a pick-up event.
      --cliffsDetected;
    }
  }
  
  if (isPickedUp) {
    if (cliffsDetected > 2) {
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.Update.CancelDueToPickup", "");
      CancelSelf();
    } else {
      PRINT_PERIODIC_CH_INFO(5, "Behaviors",
                             "BehaviorReactToCliff.Update.PossibleFalsePickUpDetected",
                             "Only %u cliff sensors are detecting cliffs, but pick-up detected.",
                             cliffsDetected);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  const bool alreadyActivated = IsActivated();
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::RobotStopped: {
      if (event.GetData().Get_RobotStopped().reason != StopReason::CLIFF) {
        break;
      }
      
      _dVars.quitReaction = false;
      _dVars.gotStop = true;
      ++_dVars.persistent.numStops;

      // Record triggered cliff sensor value(s) and compare to what they are when wheels
      // stop moving. If the values are higher, the cliff is suspicious and we should quit.
      // NOTE(GB): Due to the delay between the latest cliff sensor data and the RobotStopped
      // event, the raw data values at the time of retrieval may not register a cliff being
      // detected, since the latest cliff data can arrive up to ~30 ms after the message arrives.
      const auto& cliffComp = robotInfo.GetCliffSensorComponent();
      const auto& cliffData = cliffComp.GetCliffDataRaw();
      std::copy(cliffData.begin(), cliffData.end(), _dVars.persistent.cliffValsAtStart.begin());
      PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.CliffValsAtStart", 
                    "%u %u %u %u (alreadyActivated: %d)", 
                    _dVars.persistent.cliffValsAtStart[0], 
                    _dVars.persistent.cliffValsAtStart[1],
                    _dVars.persistent.cliffValsAtStart[2],
                    _dVars.persistent.cliffValsAtStart[3],
                    alreadyActivated);

      if (alreadyActivated) {
        CancelDelegates(false);
        OnBehaviorActivated();
      }
      break;
    }
    case EngineToGameTag::RobotOffTreadsStateChanged:
    {
      const auto treadsState = event.GetData().Get_RobotOffTreadsStateChanged().treadsState;
      const bool cliffsDetected = GetBEI().GetRobotInfo().GetCliffSensorComponent().IsCliffDetected();
      
      if (treadsState == OffTreadsState::OnTreads && cliffsDetected) {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToCliff.AlwaysHandleInScope", "Possibly put down on cliff");
        _dVars.persistent.putDownOnCliff = true;
      } else {
        _dVars.persistent.putDownOnCliff = false;
      }
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorReactToCliff.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorReactToCliff::GetCliffReactAction(uint8_t cliffDetectedFlags)
{
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));

  IActionRunner* action = nullptr;

  PRINT_CH_INFO("Behaviors", "ReactToCliff.GetCliffReactAction.CliffsDetected", "0x%x", cliffDetectedFlags);

  // Play reaction animation based on triggered sensor(s)
  // Possibly supplement with "dramatic" reaction which involves
  // turning towards the cliff and backing up in a scared/shocked fashion.
  AnimationTrigger anim;
  char proceduralDriveDirection = '\0'; // if empty, then no procedural drive selected
  switch (cliffDetectedFlags) {
    case (FL | FR):
      // Hit cliff straight-on. Play stop reaction and move on
      anim = AnimationTrigger::ReactToCliffFront;
      break;
    case FL:
      // Play stop reaction animation and turn CCW a bit
      anim = AnimationTrigger::ReactToCliffFrontLeft;
      proceduralDriveDirection = 'R';
      break;
    case FR:
      // Play stop reaction animation and turn CW a bit
      anim = AnimationTrigger::ReactToCliffFrontRight;
      proceduralDriveDirection = 'L';
      break;
    case BL:
      // Drive forward and turn CCW to face the cliff
      anim = AnimationTrigger::ReactToCliffBackLeft;
      break;
    case BR:
      // Drive forward and turn CW to face the cliff
      anim = AnimationTrigger::ReactToCliffBackRight;
      break;  
    case (BL | BR):
      // Hit cliff straight-on driving backwards. Flip around to face the cliff.
      anim = AnimationTrigger::ReactToCliffBack;
      break;
    default:
      // This is some scary configuration that we probably shouldn't move from.
      return nullptr;
  }

  if(proceduralDriveDirection != '\0' ) {
    // compound action is:
    // in parallel:
    //   + in serial:
    //      + fixed delay for animation to sync up with procedural motion
    //      + drive in an arc
    //      + wait until animation finishes (if shorter duration)
    //   + play appropriate face/lift/audio animation
    CompoundActionParallel* compoundAction = new CompoundActionParallel();
    std::weak_ptr<IActionRunner> animHandle = compoundAction->AddAction(new TriggerLiftSafeAnimationAction(anim));

    // compute path based on the curvature desired
    Planning::Path arcPath;
    const Pose3d& robot = GetBEI().GetRobotInfo().GetPose();
    f32 angle = robot.GetRotationAngle<'Z'>().ToFloat();
    if(proceduralDriveDirection == 'L') {
      arcPath.AppendArc(robot.GetTranslation().x() - kProceduralReactionArcRadius_mm * std::sin(angle), 
                        robot.GetTranslation().y() + kProceduralReactionArcRadius_mm * std::cos(angle), 
                        kProceduralReactionArcRadius_mm, 
                        robot.GetRotationAngle<'Z'>().ToFloat() - M_PI_2, 
                        -kProceduralReactionArcAngle_rad, 
                        kProceduralReactionArcSpeed_mmps, kProceduralReactionArcAccel_mmps2, kProceduralReactionArcDecel_mmps2);
    } else {
      arcPath.AppendArc(robot.GetTranslation().x() + kProceduralReactionArcRadius_mm * std::sin(angle), 
                        robot.GetTranslation().y() - kProceduralReactionArcRadius_mm * std::cos(angle), 
                        kProceduralReactionArcRadius_mm, 
                        robot.GetRotationAngle<'Z'>().ToFloat() + M_PI_2, 
                        kProceduralReactionArcAngle_rad, 
                        kProceduralReactionArcSpeed_mmps, kProceduralReactionArcAccel_mmps2, kProceduralReactionArcDecel_mmps2);
    }

    // drive action in parallel
    CompoundActionSequential* driveAndWait = new CompoundActionSequential();
    auto wait = new WaitAction(0.75f); // fixed delay that helps the animation + driving line up better
    auto drive = new DrivePathAction(arcPath);
    auto waitAnim = new WaitForLambdaAction([animHandle](Robot& robot)->bool{
      if(!animHandle.expired()) {
        auto animShared = animHandle.lock();
        return animShared->GetState() != ActionResult::RUNNING;
      }
      return true;
    },2.0f);
    wait->SetTracksToLock((u8)AnimTrackFlag::BODY_TRACK);
    drive->SetTracksToLock((u8)AnimTrackFlag::BODY_TRACK);
    waitAnim->SetTracksToLock((u8)AnimTrackFlag::BODY_TRACK);
    driveAndWait->AddAction(wait); // let the robot look left before moving
    driveAndWait->AddAction(drive);
    driveAndWait->AddAction(waitAnim);

    compoundAction->AddAction(driveAndWait);

    action = compoundAction;
  } else {
    action = new TriggerLiftSafeAnimationAction(anim);
  }

  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToFaceAndBackAwayCliff()
{
  DEBUG_SET_STATE(FaceAndBackAwayCliff);
  IActionRunner* action = nullptr;

  // determine if we need to face the cliff before playing the backup action
  Pose3d cliffPose = GetCliffPoseToLookAt();
  Pose3d cliffPoseWrtRobot;
  cliffPose.GetWithRespectTo(GetBEI().GetRobotInfo().GetPose(), cliffPoseWrtRobot);
  Point3f cliffHeading = cliffPoseWrtRobot.GetTranslation();
  cliffHeading.MakeUnitLength();
  f32 angularDistanceToCliff = std::acos(cliffHeading.x());
  if( angularDistanceToCliff > M_PI_4_F ) {
    auto compoundAction = new CompoundActionSequential();
    // Turn to face cliff
    auto turnAction = new TurnTowardsPoseAction(cliffPose);
    turnAction->SetMaxPanSpeed(DEG_TO_RAD(kBodyTurnSpeedForCliffSearch_degPerSec)); // no fast turning near cliffs
    compoundAction->AddAction(turnAction);
    // Cliff reaction animation that causes the robot to backup about 8cm
    compoundAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliff));
    action = compoundAction;
  } else {
    action = new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliff);
  }

  DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToRecoveryBackup);
}

} // namespace Vector
} // namespace Anki
