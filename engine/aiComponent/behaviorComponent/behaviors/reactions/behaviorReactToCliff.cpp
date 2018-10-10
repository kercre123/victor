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
  const char* kEventFlagTimeoutKey = "eventFlagTimeout_sec";

  // If the value of the cliff when it started stopping is
  // this much less than the value when it stopped, then
  // the cliff is considered suspicious (i.e. something like
  // carpet) and the reaction is aborted.
  // In general you'd expect the start value to be _higher_
  // that the stop value if it's true cliff, but we
  // use some margin here to account for sensor noise.
  static const u16   kSuspiciousCliffValDiff = 20;

  // minimum number of images with edge detection activated
  const int kNumImagesToWaitForEdges = 10;

  // rate of turning the robot while searching for cliffs with vision
  const f32 kBodyTurnSpeedForCliffSearch_degPerSec = 120.0f;

  // Cooldown for playing more dramatic react to cliff reaction
  CONSOLE_VAR(float, kDramaticReactToCliffCooldown_sec, CONSOLE_GROUP, 3 * 60.f);

  // If this many RobotStopped messages are received
  // while activated, then just give up and go to StuckOnEdge.
  // It's probably too dangerous to keep trying anything
  CONSOLE_VAR(uint32_t, kMaxNumRobotStopsBeforeGivingUp, CONSOLE_GROUP, 5);

  // whether this experimental feature whereby the robot uses vision
  // to extend known cliffs via edge detection is active
  CONSOLE_VAR(bool, kEnableVisualCliffExtension, CONSOLE_GROUP, false);
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
  
  eventFlagTimeout_sec = JsonTools::ParseFloat(config, kEventFlagTimeoutKey, debugName);
  // Verify that the stop event timeout limit is valid.
  if (!ANKI_VERIFY(Util::IsFltGEZero(eventFlagTimeout_sec),
                   (debugName + ".InvalidEventFlagTimeout").c_str(),
                   "Value should always be greater than or equal to 0.0 (not %.2f). Making positive.",
                   eventFlagTimeout_sec))
  {
    eventFlagTimeout_sec *= -1.0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCliff::DynamicVariables::DynamicVariables()
{
  quitReaction = false;
  gotStop = false;
  putDownOnCliff = false;
  wantsToBeActivated = false;
  hasTargetCliff = false;

  cliffPose.ClearParent();
  cliffPose = Pose3d(); // identity

  const auto kInitVal = std::numeric_limits<u16>::max();
  std::fill(persistent.cliffValsAtStart.begin(), persistent.cliffValsAtStart.end(), kInitVal);
  persistent.numStops = 0;
  persistent.lastStopTime_sec = 0.f;
  persistent.lastPutDownOnCliffTime_sec = 0.f;
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCliff::WantsToBeActivatedBehavior() const
{
  return _dVars.gotStop || _dVars.wantsToBeActivated || _dVars.putDownOnCliff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorActivated()
{
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
  auto callbackFunc = &BehaviorReactToCliff::TransitionToPlayingCliffReaction;

  WaitForLambdaAction* waitForStopAction = new WaitForLambdaAction(waitForStopLambda);
  DelegateIfInControl(waitForStopAction, callbackFunc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToStuckOnEdge()
{
  DEBUG_SET_STATE(StuckOnEdge);

  ANKI_VERIFY(_iConfig.stuckOnEdgeBehavior->WantsToBeActivated(),
              "BehaviorReactToCliff.TransitionToStuckOnEdge.DoesNotWantToBeActivated", 
              "");
  DelegateIfInControl(_iConfig.stuckOnEdgeBehavior.get());
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
    DASMSG_SET(i1, _dVars.persistent.cliffValsAtStart[0], "Sensor value 1");
    DASMSG_SET(i2, _dVars.persistent.cliffValsAtStart[1], "Sensor value 2");
    DASMSG_SET(i3, _dVars.persistent.cliffValsAtStart[2], "Sensor value 3");
    DASMSG_SET(i4, _dVars.persistent.cliffValsAtStart[3], "Sensor value 4");
    DASMSG_SEND();
  }

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::ReactedToCliff);
  
  if(ShouldStreamline()){
    TransitionToBackingUp();
  } else {
    Anki::Util::sInfo("robot.cliff_detected", {}, "");

    auto cliffDetectedFlags = GetBEI().GetRobotInfo().GetCliffSensorComponent().GetCliffDetectedFlags();
    if (cliffDetectedFlags == 0) {
      // For some reason no cliffs are detected. Just leave the reaction.
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.NoCliffs", "");
      CancelSelf();
      return;
    }

    // Did we get too many RobotStopped messages for this
    // activation of the behavior? Must be in a very "cliffy" area.
    // Just go to StuckOnEdge to be safe.
    if (_dVars.persistent.numStops > kMaxNumRobotStopsBeforeGivingUp) {
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.TooManyRobotStops", "");
      TransitionToStuckOnEdge();
      return;
    }

    // Get the pre-react action/animation to play
    auto action = GetCliffReactAction(cliffDetectedFlags);
    if (action == nullptr) {
      // No action was returned because the detected cliffs represent 
      // a precarious situation so just delegate to StuckOnEdge.
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToPlayingCliffReaction.StuckOnEdge", 
                       "%x", 
                       cliffDetectedFlags);
      TransitionToStuckOnEdge();
    } else {
      DelegateIfInControl(action, &BehaviorReactToCliff::TransitionToBackingUp);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToBackingUp()
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  // if the animation doesn't drive us backwards enough, do it manually
  if( robotInfo.GetCliffSensorComponent().IsCliffDetected() ) {
    
    // Determine whether to backup or move forward based on triggered sensor
    f32 direction = 1.f;
    if (robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FL) ||
        robotInfo.GetCliffSensorComponent().IsCliffDetected(CliffSensor::CLIFF_FR) ) {
      direction = -1.f;
    }

    PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.DoingExtraRecoveryMotion", "");
    DelegateIfInControl(new DriveStraightAction(direction * _iConfig.cliffBackupDist_mm, _iConfig.cliffBackupSpeed_mmps),
                  [this](){
                      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.ExtraRecoveryMotionComplete", "");

                      auto& cliffComponent = GetBEI().GetRobotInfo().GetCliffSensorComponent();
                      if ( cliffComponent.IsCliffDetected() ) {
                        PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToBackingUp.StillStuckOnEdge", 
                                         "%x", 
                                         cliffComponent.GetCliffDetectedFlags());
                        TransitionToStuckOnEdge();
                      } else {
                        TransitionToVisualExtendCliffs();
                      }
                  });
  } else {
    TransitionToVisualExtendCliffs();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::TransitionToVisualExtendCliffs()
{
  if(!kEnableVisualCliffExtension) {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToCliff);
    return;
  }
  
  Pose3d leftHandSide, rightHandSide;

  bool didComputeLookAtPoses = false;
  if(_dVars.hasTargetCliff) {
    // use the cached position of the observed cliff to
    // determine nearby positions to observe for possible edges
    // the positions here are a best guess of where to look
    leftHandSide.SetParent(_dVars.cliffPose);
    leftHandSide.SetTranslation({0.f, +90.f, 0.f});
    bool result = leftHandSide.GetWithRespectTo(GetBEI().GetRobotInfo().GetWorldOrigin(), leftHandSide);

    rightHandSide.SetParent(_dVars.cliffPose);
    rightHandSide.SetTranslation({0.f, -90.f, 0.f});
    result &= rightHandSide.GetWithRespectTo(GetBEI().GetRobotInfo().GetWorldOrigin(), rightHandSide);

    if(result) {
      didComputeLookAtPoses = true;
      PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToVisualCliffExtend.ObservingCliffAt", 
                      "x=%4.2f y=%4.2f",
                      _dVars.cliffPose.GetTranslation().x(),
                      _dVars.cliffPose.GetTranslation().y());
    } else {
      PRINT_NAMED_WARNING("BehaviorReactToCliff.TransitionToVisualCliffExtend.CliffPoseNotInSameTreeAsCurrentWorldOrigin","");
    }
  }

  if(!didComputeLookAtPoses) {
    // no previously set target cliff pose to look at
    // instead look at two arbitrary positions in front
    // and to either side of the robot
    leftHandSide.SetParent(GetBEI().GetRobotInfo().GetPose());
    leftHandSide.SetTranslation({60.f, +60.f, 0.f});

    rightHandSide.SetParent(GetBEI().GetRobotInfo().GetPose());
    rightHandSide.SetTranslation({60.f, -60.f, 0.f});
  }
  
  CompoundActionSequential* action = new CompoundActionSequential();

  action->AddAction(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK)); // move lift to be out of the FOV

  const auto addTurnAndObserveAction = [&action](const Pose3d& pose) -> void {
    auto turnAction = new TurnTowardsPoseAction(pose);
    turnAction->SetMaxPanSpeed(DEG_TO_RAD(kBodyTurnSpeedForCliffSearch_degPerSec));
    action->AddAction(turnAction);
    action->AddAction(new WaitForImagesAction(kNumImagesToWaitForEdges, VisionMode::DetectingOverheadEdges));
  };

  addTurnAndObserveAction(leftHandSide);
  addTurnAndObserveAction(rightHandSide);
  
  BehaviorSimpleCallback callback = [this] (void) -> void {
    PRINT_NAMED_INFO("BehaviorReactToCliff.TransitionToVisualCliffExtend.ObservationFinished", "");
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToCliff);
  };

  DelegateIfInControl(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::OnBehaviorDeactivated()
{
  // reset dvars
  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.stuckOnEdgeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCliff::BehaviorUpdate()
{
  if(!IsActivated()){
    const auto& currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (_dVars.gotStop) {
      const auto& timeSinceLastStop_sec = currentTime_sec - _dVars.persistent.lastStopTime_sec;
      if (timeSinceLastStop_sec > _iConfig.eventFlagTimeout_sec) {
        _dVars.gotStop = false;
        PRINT_NAMED_INFO("BehaviorReactToCliff.Update.IgnoreLastStopEvent", "");
      }
    }
    if (_dVars.putDownOnCliff) {
      const auto& timeSinceLastPutDownOnCliff_sec = currentTime_sec - _dVars.persistent.lastPutDownOnCliffTime_sec;
      if (timeSinceLastPutDownOnCliff_sec > _iConfig.eventFlagTimeout_sec) {
        _dVars.putDownOnCliff = false;
        PRINT_NAMED_INFO("BehaviorReactToCliff.Update.IgnoreLastPossiblePutDownOnCliffEvent", "");
      }
    }
    // Set wantsToBeActivated to effectively give the activation conditions
    // an extra tick to be evaluated.
    _dVars.wantsToBeActivated = _dVars.gotStop || _dVars.putDownOnCliff;
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
  //   PRINT_NAMED_INFO("BehaviorReactToCliff.Update.StuckOnEdge", "");
  //   _iConfig.stuckOnEdgeBehavior->WantsToBeActivated();
  //   DelegateNow(_iConfig.stuckOnEdgeBehavior.get());
  // }

  // Cancel if picked up
  const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();
  if (isPickedUp) {
    PRINT_NAMED_INFO("BehaviorReactToCliff.Update.CancelDueToPickup", "");
    CancelSelf();
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
      _dVars.persistent.lastStopTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      ++_dVars.persistent.numStops;

      // Record triggered cliff sensor value(s) and compare to what they are when wheels
      // stop moving. If the values are higher, the cliff is suspicious and we should quit.
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
        _dVars.putDownOnCliff = true;
        _dVars.persistent.lastPutDownOnCliffTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      } else {
        _dVars.putDownOnCliff = false;
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
CompoundActionSequential* BehaviorReactToCliff::GetCliffReactAction(uint8_t cliffDetectedFlags)
{
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));

  auto action = new CompoundActionSequential();

  float amountToTurn_deg = 0.f;

  // Play dramatic backup reaction once in a while
  const float currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool doDramaticReaction = currTime_sec > _dramaticCliffReactionPlayableTime_sec;

  PRINT_NAMED_INFO("ReactToCliff.GetCliffReactAction.CliffsDetected", "0x%x", cliffDetectedFlags);

  // Play reaction animation based on triggered sensor(s)
  // Possibly supplement with "dramatic" reaction which involves
  // turning towards the cliff and backing up in a scared/shocked fashion.
  switch (cliffDetectedFlags) {
    case (FL | FR):
      // Hit cliff straight-on. Play stop reaction and move on
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffFront));
      break;
    case FL:
      // Play stop reaction animation and turn CCW a bit
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffFrontLeft));
      amountToTurn_deg = 15.f;
      break;
    case FR:
      // Play stop reaction animation and turn CW a bit
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffFrontRight));
      amountToTurn_deg = -15.f;
      break;
    case BL:
      // Drive forward and turn CCW to face the cliff
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffBackLeft));
      amountToTurn_deg = 135.f;
      break;
    case BR:
      // Drive forward and turn CW to face the cliff
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffBackRight));
      amountToTurn_deg = -135.f;
      break;  
    case (BL | BR):
      // Hit cliff straight-on driving backwards. Flip around to face the cliff.
      action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToCliffBack));
      amountToTurn_deg = 180.f;
      break;
    default:
      // This is some scary configuration that we probably shouldn't move from.
      delete(action);
      return nullptr;
  }

  
  if (doDramaticReaction) {
    PRINT_NAMED_INFO("BehaviorReactToCliff.GetCliffReactAction.DoDramaticReaction", "");

    // Turn to face cliff
    if (amountToTurn_deg != 0.f) {
      auto turnAction = new TurnInPlaceAction(DEG_TO_RAD(amountToTurn_deg), false);
      turnAction->SetAccel(MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2);
      turnAction->SetMaxSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
      action->AddAction(turnAction);
    }

    // Dramatic backup
    AnimationTrigger reactionAnim = AnimationTrigger::ReactToCliff;
    action->AddAction(new TriggerLiftSafeAnimationAction(reactionAnim));

    _dramaticCliffReactionPlayableTime_sec = currTime_sec + kDramaticReactToCliffCooldown_sec;
  }

  return action;
}

} // namespace Vector
} // namespace Anki
