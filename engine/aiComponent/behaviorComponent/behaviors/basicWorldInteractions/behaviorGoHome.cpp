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
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorWiggleOntoChargerContacts.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/navMap/mapComponent.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"

namespace Anki {
namespace Cozmo {

namespace {
  const char* kUseCliffSensorsKey        = "useCliffSensorCorrection";
  const char* kLeftTurnAnimKey           = "leftTurnAnimTrigger";
  const char* kRightTurnAnimKey          = "rightTurnAnimTrigger";
  const char* kBackupStartAnimKey        = "backupStartAnimTrigger";
  const char* kBackupEndAnimKey          = "backupEndAnimTrigger";
  const char* kBackupLoopAnimKey         = "backupLoopAnimTrigger";
  const char* kRaiseLiftAnimKey          = "raiseLiftAnimTrigger";
  const char* kNuzzleAnimKey             = "nuzzleAnimTrigger";
  const char* kDriveToRetryCountKey      = "driveToRetryCount";
  const char* kTurnToDockRetryCountKey   = "turnToDockRetryCount";
  const char* kMountChargerRetryCountKey = "mountChargerRetryCount";
  
  // Pre-action pose angle tolerance to use for driving to the charger. This
  // can be a bit looser than the default, since the robot can successfully
  // dock with the charger even if he's not precisely at the pre-action pose.
  const float kDriveToChargerPreActionPoseAngleTol_rad = DEG_TO_RAD(15.f);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{  
  useCliffSensorCorrection = JsonTools::ParseBool(config, kUseCliffSensorsKey, debugName);
  
  JsonTools::GetCladEnumFromJSON(config, kLeftTurnAnimKey,    leftTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kRightTurnAnimKey,   rightTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kBackupStartAnimKey, backupStartAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kBackupEndAnimKey,   backupEndAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kBackupLoopAnimKey,  backupLoopAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kRaiseLiftAnimKey,   raiseLiftAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kNuzzleAnimKey,      nuzzleAnimTrigger, debugName);
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
    kBackupStartAnimKey,
    kBackupEndAnimKey,
    kBackupLoopAnimKey,
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
  _dVars = DynamicVariables();
  
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  PushDrivingAnims();
  
  _dVars.chargerID = object->GetID();
  
  // Check here if we're carrying an object and put it down next to the charger if so
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    TransitionToPlacingCubeOnGround();
  } else {
    TransitionToCheckDockingArea();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorDeactivated()
{
  PopDrivingAnims();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToCheckDockingArea()
{
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
  const auto* charger = dynamic_cast<const Charger*>(GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger));
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.TransitionToDriveToCharger.NullCharger", "Null charger!");
    return;
  }

  // We always just clear the area in front of the charger of
  // obstacles. If there is still an obstacle there, we will
  // encounter it when we drive up to it.
  const auto lastTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  Poly2f chargerDockingPoly;
  chargerDockingPoly.ImportQuad2d(charger->GetDockingAreaQuad());
  GetBEI().GetMapComponent().InsertData(chargerDockingPoly,
                                        MemoryMapData(MemoryMapTypes::EContentType::ClearOfObstacle, lastTimestamp));
  
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
                                                "Path planning timed out probably due to prox obstacles - clearing "
                                                "an area of the nav map and trying again. NOTE: This should be removed "
                                                "once path planning is improved and times out less.");
                            ClearNavMapUpToCharger();
                          }
                          TransitionToDriveToCharger();
                        } else {
                          // Either out of retries or we got another failure type
                          ActionFailure();
                        }
                      });
}

  
void BehaviorGoHome::TransitionToCheckPreTurnPosition()
{
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
  // Play the animations to raise lift, then mount the charger
  auto* action = new CompoundActionSequential();
  action->AddAction(new TriggerAnimationAction(_iConfig.raiseLiftAnimTrigger));
  action->AddAction(new MountChargerAction(_dVars.chargerID, _iConfig.useCliffSensorCorrection));
  
  DelegateIfInControl(action,
                      [this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        if (resultCategory == ActionResultCategory::SUCCESS) {
                          TransitionToPlayingNuzzleAnim();
                        } else if ((resultCategory == ActionResultCategory::RETRY) &&
                                   (_dVars.mountChargerRetryCount++ < _iConfig.mountChargerRetryCount)) {
                          // Simply go back to the starting pose, which will allow visual
                          // verification to happen again, etc.
                          TransitionToDriveToCharger();
                        } else {
                          // Either out of retries or we got another failure type.
                          // If the robot did not end the action on the charger, then clear
                          // the charger from the world since we clearly do not know where it is
                          // Note: Here is where we could capture CHARGER_UNPLUGGED for
                          //       potential messaging up to the app.
                          const bool removeChargerFromBlockworld = (result == ActionResult::NOT_ON_CHARGER_ABORT);
                          ActionFailure(removeChargerFromBlockworld);
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToPlayingNuzzleAnim()
{
  // Remove driving animations
  PopDrivingAnims();
  
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.nuzzleAnimTrigger),
                      &BehaviorGoHome::TransitionToOnChargerCheck);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToOnChargerCheck()
{
  // If we've somehow wiggled off the charge contacts, try the 'wiggle'
  // behavior to get us back onto the contacts
  const bool wiggleWantsToRun = _iConfig.wiggleOntoChargerBehavior->WantsToBeActivated();
  if (!GetBEI().GetRobotInfo().IsOnChargerContacts() &&
      wiggleWantsToRun) {
    DelegateIfInControl(_iConfig.wiggleOntoChargerBehavior.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::ActionFailure(const bool removeChargerFromBlockWorld)
{
  PRINT_NAMED_WARNING("BehaviorGoHome.ActionFailure",
                      "BehaviorGoHome ending due to an action failure. %s",
                      removeChargerFromBlockWorld ? "Removing charger from block world." : "");
  
  if (removeChargerFromBlockWorld) {
    BlockWorldFilter deleteFilter;
    deleteFilter.AddAllowedID(_dVars.chargerID);
    GetBEI().GetBlockWorld().DeleteLocatedObjects(deleteFilter);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::PushDrivingAnims()
{
  DEV_ASSERT(!_dVars.drivingAnimsPushed, "BehaviorGoHome.PushDrivingAnims.AnimsAlreadyPushed");
  
  if (!_dVars.drivingAnimsPushed) {
    auto& drivingAnimHandler = GetBEI().GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.PushDrivingAnimations({_iConfig.backupStartAnimTrigger,
                                              _iConfig.backupLoopAnimTrigger,
                                              _iConfig.backupEndAnimTrigger},
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

} // namespace Cozmo
} // namespace Anki
