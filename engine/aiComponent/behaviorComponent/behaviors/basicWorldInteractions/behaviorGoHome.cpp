/**
 * File: behaviorGoHome.cpp
 *
 * Author: Matt Michini
 * Created: 11/1/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorGoHome.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/chargerActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorClearChargerArea.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRequestToGoHome.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorWiggleOntoChargerContacts.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/navMap/mapComponent.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/types/behaviorComponent/behaviorStats.h"

#include "util/logging/DAS.h"

#define LOG_FUNCTION_NAME() PRINT_CH_INFO("Behaviors", "BehaviorGoHome", "BehaviorGoHome.%s", __func__);

namespace Anki {
namespace Vector {

namespace {
  const char* kUseCliffSensorsKey        = "useCliffSensorCorrection";
  const char* kLeftTurnAnimKey           = "leftTurnAnimTrigger";
  const char* kRightTurnAnimKey          = "rightTurnAnimTrigger";
  const char* kDrivingStartAnimKey       = "drivingStartAnimTrigger";
  const char* kDrivingEndAnimKey         = "drivingEndAnimTrigger";
  const char* kDrivingLoopAnimKey        = "drivingLoopAnimTrigger";
  const char* kRaiseLiftAnimKey          = "raiseLiftAnimTrigger";
  const char* kNuzzleAnimKey             = "nuzzleAnimTrigger";
  const char* kDriveToRetryCountKey      = "driveToRetryCount";
  const char* kTurnToDockRetryCountKey   = "turnToDockRetryCount";
  const char* kMountChargerRetryCountKey = "mountChargerRetryCount";
  
  // Pre-action pose angle tolerance to use for driving to the charger. This
  // can be a bit looser than the default, since the robot can successfully
  // dock with the charger even if he's not precisely at the pre-action pose.
  const float kDriveToChargerPreActionPoseAngleTol_rad = DEG_TO_RAD(15.f);
  
  // If we have been activated too many times within a short window of time,
  // then just delegate to the RequestToGoHome behavior instead of continuing
  // with the GoHome behavior, since we may be stuck in a loop.
  const float kRepeatedActivationCheckWindow_sec = 60.f;
  const size_t kNumRepeatedActivationsAllowed = 3;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{  
  useCliffSensorCorrection = JsonTools::ParseBool(config, kUseCliffSensorsKey, debugName);
  
  JsonTools::GetCladEnumFromJSON(config, kLeftTurnAnimKey,     leftTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kRightTurnAnimKey,    rightTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kDrivingStartAnimKey, drivingStartAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kDrivingEndAnimKey,   drivingEndAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kDrivingLoopAnimKey,  drivingLoopAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kRaiseLiftAnimKey,    raiseLiftAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kNuzzleAnimKey,       nuzzleAnimTrigger, debugName);
  homeFilter = std::make_unique<BlockWorldFilter>();
  driveToRetryCount = JsonTools::ParseInt32(config, kDriveToRetryCountKey, debugName);
  turnToDockRetryCount = JsonTools::ParseInt32(config, kTurnToDockRetryCountKey, debugName);
  mountChargerRetryCount = JsonTools::ParseInt32(config, kMountChargerRetryCountKey, debugName);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::BehaviorGoHome(const Json::Value& config)
  : ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig = InstanceConfig(config, debugName);
  
  // Set up block world filter for finding Home object
  _iConfig.homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _iConfig.homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kUseCliffSensorsKey,
    kLeftTurnAnimKey,
    kRightTurnAnimKey,
    kDrivingStartAnimKey,
    kDrivingEndAnimKey,
    kDrivingLoopAnimKey,
    kRaiseLiftAnimKey,
    kNuzzleAnimKey,
    kDriveToRetryCountKey,
    kTurnToDockRetryCountKey,
    kMountChargerRetryCountKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.clearChargerAreaBehavior.get());
  delegates.insert(_iConfig.requestHomeBehavior.get());
  delegates.insert(_iConfig.wiggleOntoChargerBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorClearChargerArea>(BEHAVIOR_ID(ClearChargerArea),
                                                           BEHAVIOR_CLASS(ClearChargerArea),
                                                           _iConfig.clearChargerAreaBehavior);
  DEV_ASSERT(_iConfig.clearChargerAreaBehavior != nullptr,
             "BehaviorGoHome.InitBehavior.NullClearChargerAreaBehavior");
  BC.FindBehaviorByIDAndDowncast<BehaviorRequestToGoHome>(BEHAVIOR_ID(RequestHomeBecauseStuck),
                                                          BEHAVIOR_CLASS(RequestToGoHome),
                                                          _iConfig.requestHomeBehavior);
  DEV_ASSERT(_iConfig.requestHomeBehavior != nullptr,
             "BehaviorGoHome.InitBehavior.NullRequestHomeBehavior");
  BC.FindBehaviorByIDAndDowncast<BehaviorWiggleOntoChargerContacts>(BEHAVIOR_ID(WiggleBackOntoChargerFromPlatform),
                                                                    BEHAVIOR_CLASS(WiggleOntoChargerContacts),
                                                                    _iConfig.wiggleOntoChargerBehavior);
  DEV_ASSERT(_iConfig.wiggleOntoChargerBehavior != nullptr,
             "BehaviorGoHome.InitBehavior.NullWiggleOntoChargerBehavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGoHome::WantsToBeActivatedBehavior() const
{
  const bool isOnChargerContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();
  if (isOnChargerContacts) {
    return false;
  }
  
  // If we have a located Home/charger, then we can be activated.
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_iConfig.homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  return hasAHome;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorActivated()
{
  LOG_FUNCTION_NAME();
  
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  
  // Have we been activated a lot recently?
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& times = _dVars.persistent.activatedTimes;
  times.insert(now_sec);
  times.erase(times.begin(),
              times.lower_bound(now_sec - kRepeatedActivationCheckWindow_sec));
  if (times.size() > kNumRepeatedActivationsAllowed) {
    PRINT_NAMED_WARNING("BehaviorGoHome.OnBehaviorActivated.RepeatedlyActivated",
                        "We have been activated %zu times in the past %.1f seconds, so instead of continuing "
                        "with this behavior, we are playing the failure anim and exiting.",
                        times.size(), kRepeatedActivationCheckWindow_sec);
    // Clear the list of activated times (so that we don't get stuck in a loop here) and play the 'failure' anim
    _dVars.persistent.activatedTimes.clear();
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ChargerDockingFailure));
    return;
  }
  
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  PushDrivingAnims();
  
  _dVars.chargerID = object->GetID();
  
  // Clear all of the prox obstacles since stale/old prox obstacles
  // can wreak havoc on the robot's ability to get home. Hopefully
  // we can find a more elegant way to clear prox obstacles that are
  // causing the robot to fail to plan a path to the charger (VIC-2978)
  GetBEI().GetMapComponent().RemoveAllProxObstacles();
  
  // First turn toward the charger. This will hopefully update its pose
  // to be more accurate before interacting with it.
  auto* turnToAction = new TurnTowardsObjectAction(_dVars.chargerID);
  DelegateIfInControl(turnToAction,
                      [this]() {
                        // We don't care if the turn action failed or not, just
                        // continue with the behavior. Check if we're carrying
                        // an object and put it down next to the charger if so
                        const auto& robotInfo = GetBEI().GetRobotInfo();
                        if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
                          TransitionToPlacingCubeOnGround();
                        } else {
                          TransitionToCheckDockingArea();
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorDeactivated()
{
  PopDrivingAnims();
  
  // If we had a clear success or failure, log it here
  if (_dVars.HasResult()) {
    DASMSG(go_home_result, "go_home.result", "Result of GoHome behavior");
    DASMSG_SET(i1, _dVars.HasSucceeded(), "Success or failure to get onto the charger (1 for success, 0 for failure)");
    DASMSG_SEND();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToCheckDockingArea()
{
  LOG_FUNCTION_NAME();
  
  const bool clearChargerWantsToRun = _iConfig.clearChargerAreaBehavior->WantsToBeActivated();
  if (clearChargerWantsToRun) {
    DelegateIfInControl(_iConfig.clearChargerAreaBehavior.get(),
                        &BehaviorGoHome::TransitionToDriveToCharger);
  } else {
    // ClearChargerArea behavior does not want to run,
    // so charger docking area must be clear of cubes
    TransitionToDriveToCharger();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToPlacingCubeOnGround()
{
  LOG_FUNCTION_NAME();
  
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.TransitionToPlacingCubeOnGround", "Null charger!");
    return;
  }
  
  // Place the cube on the ground next to the charger
  Pose3d cubePlacementPose;
  cubePlacementPose.SetTranslation(Vec3f(20.f, 100.f, 0.f));
  cubePlacementPose.SetParent(charger->GetPose());
  DelegateIfInControl(new PlaceObjectOnGroundAtPoseAction(cubePlacementPose),
                      [this](ActionResult result) {
                        const auto& robotInfo = GetBEI().GetRobotInfo();
                        if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
                          // Still carrying an object. Simply turn away from the charger
                          // and place it on the ground right there. This will hopefully
                          // get it out of the way.
                          const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
                          if (charger == nullptr) {
                            PRINT_NAMED_ERROR("BehaviorGoHome.TransitionToPlacingCubeOnGroundCallback.NullCharger", "Null charger!");
                            return;
                          }
                          auto* compoundAction = new CompoundActionSequential();
                          const float turnAngle = atan2f(robotInfo.GetPose().GetTranslation().y() - charger->GetPose().GetTranslation().y(),
                                                         robotInfo.GetPose().GetTranslation().x() - charger->GetPose().GetTranslation().x());
                          compoundAction->AddAction(new TurnInPlaceAction(turnAngle, true));
                          compoundAction->AddAction(new PlaceObjectOnGroundAction());
                          DelegateIfInControl(compoundAction,
                                              &BehaviorGoHome::TransitionToDriveToCharger);
                        } else {
                          TransitionToDriveToCharger();
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToDriveToCharger()
{
  LOG_FUNCTION_NAME();
  
  const auto* charger = dynamic_cast<const Charger*>(GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger));
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.TransitionToDriveToCharger.NullCharger", "Null charger!");
    return;
  }

  // We always just clear the area in front of the charger of obstacles and cliffs. If there is still an obstacle or
  // cliff there, we will encounter it when we drive up to it.
  const auto lastTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  Poly2f chargerDockingPoly;
  chargerDockingPoly.ImportQuad2d(charger->GetDockingAreaQuad());
  GetBEI().GetMapComponent().InsertData(chargerDockingPoly,
                                        MemoryMapData(MemoryMapTypes::EContentType::ClearOfCliff, lastTimestamp));
  
  auto* driveToAction = new DriveToObjectAction(_dVars.chargerID, PreActionPose::ActionType::DOCKING);
  driveToAction->SetPreActionPoseAngleTolerance(kDriveToChargerPreActionPoseAngleTol_rad);
  DelegateIfInControl(driveToAction,
                      [this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        if (resultCategory == ActionResultCategory::SUCCESS) {
                          TransitionToCheckPreTurnPosition();
                        } else if (result == ActionResult::VISUAL_OBSERVATION_FAILED) {
                          // If visual observation failed, then we've successfully gotten to the charger
                          // pre-action pose, but it is no longer there. Delete the charger from the map.
                          PRINT_NAMED_WARNING("BehaviorGoHome.TransitionToDriveToCharger.DeletingCharger",
                                              "Deleting charger with ID %d since visual verification failed",
                                              _dVars.chargerID.GetValue());
                          const bool removeChargerFromBlockworld = true;
                          ActionFailure(removeChargerFromBlockworld);
                        } else if ((_dVars.driveToRetryCount++ < _iConfig.driveToRetryCount) &&
                                   ((resultCategory == ActionResultCategory::RETRY) ||
                                    (result == ActionResult::PATH_PLANNING_FAILED_ABORT))) {
                          if (result == ActionResult::PATH_PLANNING_FAILED_ABORT) {
                            PRINT_NAMED_WARNING("BehaviorGoHome.TransitionToDriveToCharger.PathPlanningTimedOut",
                                                "Path planning timed out possibly due to prox obstacles or there is no "
                                                "valid path from our location. Clearing an area of the nav map, turning in "
                                                "place a bit, then trying again.");
                            ClearNavMapUpToCharger();
                            // Do a small point turn to give the planner a new starting point, so that hopefully it does
                            // not time out again.
                            const float direction = GetRNG().RandBool() ? 1.f : -1.f;
                            const float angle = direction * GetRNG().RandDblInRange(M_PI_4_F, M_PI_2_F);
                            const bool isAbsolute = false;
                            DelegateIfInControl(new TurnInPlaceAction(angle, isAbsolute), [this](ActionResult res) {
                              TransitionToDriveToCharger();
                            });
                          } else {
                            TransitionToDriveToCharger();
                          }
                        } else {
                          // Either out of retries or we got another failure type
                          ActionFailure();
                        }
                      });
}

  
void BehaviorGoHome::TransitionToCheckPreTurnPosition()
{
  LOG_FUNCTION_NAME();
  
  // Check to make sure we are in a safe position to begin the 180
  // degree turn. We could have been bumped, or the charger could
  // have moved. This is the last chance to verify that we're in a
  // good position to start the docking sequence.
  
  // Look forward, then wait a brief time to acquire some more images and
  // a more accurate pose of the charger. Then verify pose.
  const float kWaitBeforeVerifyTime_sec = 0.3f;
  auto* compoundAction = new CompoundActionSequential();
  compoundAction->AddAction(new MoveHeadToAngleAction(0.f));
  compoundAction->AddAction(new WaitAction(kWaitBeforeVerifyTime_sec));
  compoundAction->AddAction(new VisuallyVerifyObjectAction(_dVars.chargerID));

  auto checkPoseFunc = [this]() -> bool {
    const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
    if (charger == nullptr) {
      return false;
    }
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d robotPoseWrtCharger;
    if (!robotPose.GetWithRespectTo(charger->GetPose(), robotPoseWrtCharger)) {
      return false;
    }
    const auto& xWrtCharger = robotPoseWrtCharger.GetTranslation().x();
    const auto& yWrtCharger = robotPoseWrtCharger.GetTranslation().y();
    const auto& angleWrtCharger = robotPoseWrtCharger.GetRotation().GetAngleAroundZaxis();
    
    const float kIdealXWrtCharger_mm = -40.f;
    const float kIdealYWrtCharger_mm = 0.f;
    const float kIdealAngleWrtCharger_rad = 0.f;
    
    const float kMaxXError_mm = 20.f;
    const float kMaxYError_mm = 25.f;
    const float kMaxAngularError_rad = DEG_TO_RAD(20.f);
    
    const bool poseOk = Util::IsNear(xWrtCharger, kIdealXWrtCharger_mm, kMaxXError_mm) &&
                        Util::IsNear(yWrtCharger, kIdealYWrtCharger_mm, kMaxYError_mm) &&
                        angleWrtCharger.IsNear(kIdealAngleWrtCharger_rad, kMaxAngularError_rad);
    
    if (!poseOk) {
      PRINT_NAMED_WARNING("BehaviorGoHome.TransitionToCheckPreTurnPosition.NotInPosition",
                          "Ended up not in a good position to commence the turn to begin docking. RobotPoseWrtCharger {%.1f, %.1f, %.1f deg}",
                          xWrtCharger, yWrtCharger, angleWrtCharger.getDegrees());
    }
    
    return poseOk;
  };
  
  DelegateIfInControl(compoundAction,
                      [checkPoseFunc, this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        const bool poseOk = checkPoseFunc();
                        if ((resultCategory == ActionResultCategory::SUCCESS) && poseOk) {
                          TransitionToTurn();
                        } else if (_dVars.turnToDockRetryCount++ < _iConfig.turnToDockRetryCount) {
                          // Simply go back to the starting pose, which will allow visual
                          // verification to happen again, etc.
                          TransitionToDriveToCharger();
                        } else {
                          // Out of retries
                          ActionFailure();
                        }
                      });
}
  


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToTurn()
{
  LOG_FUNCTION_NAME();
  
  // Turn to align with the charger
  DelegateIfInControl(new TurnToAlignWithChargerAction(_dVars.chargerID,
                                                       _iConfig.leftTurnAnimTrigger,
                                                       _iConfig.rightTurnAnimTrigger),
                      [this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        if (resultCategory == ActionResultCategory::SUCCESS) {
                          TransitionToMountCharger();
                        } else if ((resultCategory == ActionResultCategory::RETRY) &&
                                   (_dVars.turnToDockRetryCount++ < _iConfig.turnToDockRetryCount)) {
                          // Simply go back to the starting pose, which will allow visual
                          // verification to happen again, etc.
                          TransitionToDriveToCharger();
                        } else {
                          // Either out of retries or we got another failure type
                          ActionFailure();
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToMountCharger()
{
  LOG_FUNCTION_NAME();
  
  // Play the animations to raise lift, then mount the charger
  auto* action = new CompoundActionSequential();
  action->AddAction(new TriggerAnimationAction(_iConfig.raiseLiftAnimTrigger));
  auto* mountAction = new MountChargerAction(_dVars.chargerID, _iConfig.useCliffSensorCorrection);
  mountAction->SetDockingAnimTriggers(_iConfig.drivingStartAnimTrigger,
                                      _iConfig.drivingLoopAnimTrigger,
                                      _iConfig.drivingEndAnimTrigger);
  action->AddAction(mountAction);
  
  DelegateIfInControl(action,
                      [this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        if (resultCategory == ActionResultCategory::SUCCESS) {
                          GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::MountedCharger);
                          _dVars.SetSucceeded(true);
                          TransitionToPlayingNuzzleAnim();
                        } else if ((resultCategory == ActionResultCategory::RETRY) &&
                                   (_dVars.mountChargerRetryCount++ < _iConfig.mountChargerRetryCount)) {
                          // Simply go back to the starting pose, which will allow visual
                          // verification to happen again, etc.
                          TransitionToDriveToCharger();
                        } else {
                          // Either out of retries or we got another failure type. If the charger is unplugged, drive
                          // forward off of it before ending the behavior.
                          if (result == ActionResult::CHARGER_UNPLUGGED_ABORT) {
                            // Note: Here is where we could capture CHARGER_UNPLUGGED for
                            //       potential messaging up to the app.
                            DelegateIfInControl(new DriveStraightAction(100.f,
                                                                        DEFAULT_PATH_MOTION_PROFILE.speed_mmps,
                                                                        false),
                                                [this]() {
                                                  ActionFailure(false);
                                                });
                          } else {
                            // If the robot did not end the action on the charger, then clear the charger from the
                            // world since we clearly do not know where it is.
                            const bool removeChargerFromBlockworld = (result == ActionResult::NOT_ON_CHARGER_ABORT);
                            ActionFailure(removeChargerFromBlockworld);
                          }
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToPlayingNuzzleAnim()
{
  LOG_FUNCTION_NAME();
  
  // Remove driving animations
  PopDrivingAnims();
  
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.nuzzleAnimTrigger),
                      &BehaviorGoHome::TransitionToOnChargerCheck);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToOnChargerCheck()
{
  LOG_FUNCTION_NAME();
  
  // If we've somehow wiggled off the charge contacts, try the 'wiggle'
  // behavior to get us back onto the contacts
  const bool wiggleWantsToRun = _iConfig.wiggleOntoChargerBehavior->WantsToBeActivated();
  if (!GetBEI().GetRobotInfo().IsOnChargerContacts() &&
      wiggleWantsToRun) {
    DelegateIfInControl(_iConfig.wiggleOntoChargerBehavior.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::ActionFailure(bool removeChargerFromBlockWorld)
{
  // If we are ending due to an action failure, now is a good time to check
  // how recently we have seen the charger. If we haven't seen the charger
  // in a while, just remove it from the world since it's possible that we
  // really don't know where it is. 
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  const auto nowTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  const TimeStamp_t kMaxObservedAge_ms = Util::SecToMilliSec(120.f);
  if ((charger != nullptr) &&
      (nowTimestamp > charger->GetLastObservedTime() + kMaxObservedAge_ms)) {
    removeChargerFromBlockWorld = true;
  }
  
  PRINT_NAMED_WARNING("BehaviorGoHome.ActionFailure",
                      "BehaviorGoHome had an action failure. Delegating to the request to go home. %s",
                      removeChargerFromBlockWorld ? "Removing charger from block world." : "");
  
  if (removeChargerFromBlockWorld) {
    BlockWorldFilter deleteFilter;
    deleteFilter.AddAllowedID(_dVars.chargerID);
    GetBEI().GetBlockWorld().DeleteLocatedObjects(deleteFilter);
  }
  
  // Play the "charger face" animation indicating that we have failed, then allow the behavior to exit
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ChargerDockingFailure));
  
  _dVars.SetSucceeded(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::PushDrivingAnims()
{
  DEV_ASSERT(!_dVars.drivingAnimsPushed, "BehaviorGoHome.PushDrivingAnims.AnimsAlreadyPushed");
  
  if (!_dVars.drivingAnimsPushed) {
    auto& drivingAnimHandler = GetBEI().GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.PushDrivingAnimations({_iConfig.drivingStartAnimTrigger,
                                              _iConfig.drivingLoopAnimTrigger,
                                              _iConfig.drivingEndAnimTrigger},
                                             GetDebugLabel());
    _dVars.drivingAnimsPushed = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::PopDrivingAnims()
{
  if (_dVars.drivingAnimsPushed) {
    auto& drivingAnimHandler = GetBEI().GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.RemoveDrivingAnimations(GetDebugLabel());
    _dVars.drivingAnimsPushed = false;
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::ClearNavMapUpToCharger()
{
  // Take the center point on the line between the robot and the charger,
  // and clear a 'circular' area of radius slightly larger than the line.
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.ClearNavMapUpToCharger.NullCharger", "Null charger!");
    return;
  }

  const auto& robotPoseWrtOrigin = GetBEI().GetRobotInfo().GetPose().GetWithRespectToRoot();
  const auto& chargerPoseWrtOrigin = charger->GetPose().GetWithRespectToRoot();
  
  float distanceBetween_mm = 0.f;
  if (!ComputeDistanceBetween(robotPoseWrtOrigin, chargerPoseWrtOrigin, distanceBetween_mm)) {
    DEV_ASSERT(false, "BehaviorGoHome.ClearNavMapUpToCharger.FailedComputeDistanceBetween");
  }
  
  const Point2f centerPoint((robotPoseWrtOrigin.GetTranslation().x() + chargerPoseWrtOrigin.GetTranslation().x()) / 2.f,
                            (robotPoseWrtOrigin.GetTranslation().y() + chargerPoseWrtOrigin.GetTranslation().y()) / 2.f);
  
  // create a poly that approximates a circle around the midpoint
  const float padding_mm = 40.f;
  const float radius_mm = (distanceBetween_mm + padding_mm) / 2.f;
  const int numVertices= 8;
  Poly2f pointsPoly;
  for (int i=0 ; i<numVertices ; i++) {
    const float th = i * M_TWO_PI_F / numVertices;
    const Point2f pt(centerPoint.x() + radius_mm * std::cos(th),
                     centerPoint.y() + radius_mm * std::sin(th));
    pointsPoly.push_back(pt);
  }
  
  const auto lastTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  GetBEI().GetMapComponent().InsertData(pointsPoly,
                                        MemoryMapData(MemoryMapTypes::EContentType::ClearOfObstacle, lastTimestamp));
}

} // namespace Vector
} // namespace Anki
