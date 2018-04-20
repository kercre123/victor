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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
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
  delegates.insert(_iConfig.pickupBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(BEHAVIOR_ID(PickupCube),
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickupBehavior);
  DEV_ASSERT(_iConfig.pickupBehavior != nullptr,
             "BehaviorGoHome.InitBehavior.NullPickupBehavior");
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
  // See if there are any obstacles in the area immediately in front of the charger
  const auto* charger = dynamic_cast<const Charger*>(GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger));
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.TransitionToCheckDockingArea.NullCharger", "Null charger!");
    return;
  }
  
  BlockWorldFilter cubesFilter;
  cubesFilter.AddAllowedFamily(ObjectFamily::LightCube);
  
  std::vector<const ObservableObject*> intersectingCubes;
  GetBEI().GetBlockWorld().FindLocatedIntersectingObjects(charger->GetDockingAreaQuad(),
                                                          intersectingCubes,
                                                          0.f,
                                                          cubesFilter);

  if (!intersectingCubes.empty()) {
    // There is a cube in the way. Go pick it up.
    // Just use the first cube in the list
    const auto& cubeId = intersectingCubes.front()->GetID();
    _iConfig.pickupBehavior->SetTargetID(cubeId);
    const bool pickupWantsToRun = _iConfig.pickupBehavior->WantsToBeActivated();
    DEV_ASSERT(pickupWantsToRun, "BehaviorGoHome.TransitionToCheckDockingArea.PickupDoesNotWantToRun");
    DelegateIfInControl(_iConfig.pickupBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            TransitionToPlacingCubeOnGround();
                          } else {
                            // We're not carrying a cube for some reason - just
                            // attempt to get to the charger.
                            TransitionToDriveToCharger();
                          }
                        });
  } else {
    // No cubes in the charger docking area
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
  driveToAction->SetPreActionPoseAngleTolerance(DEG_TO_RAD(20.f));
  DelegateIfInControl(driveToAction,
                      [this](ActionResult result) {
                        const auto resultCategory = IActionRunner::GetActionResultCategory(result);
                        if (resultCategory == ActionResultCategory::SUCCESS) {
                          TransitionToTurn();
                        } else if (result == ActionResult::VISUAL_OBSERVATION_FAILED) {
                          // If visual observation failed, then we've successfully gotten to the charger
                          // pre-action pose, but it is no longer there. Delete the charger from the map.
                          PRINT_NAMED_WARNING("BehaviorGoHome.TransitionToDriveToCharger.DeletingCharger",
                                              "Deleting charger with ID %d since visual verification failed",
                                              _dVars.chargerID.GetValue());
                          BlockWorldFilter deleteFilter;
                          deleteFilter.AddAllowedID(_dVars.chargerID);
                          GetBEI().GetBlockWorld().DeleteLocatedObjects(deleteFilter);
                          ActionFailure();
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
                          // Either out of retries or we got another failure type
                          // Note: Here is where we could capture CHARGER_UNPLUGGED for
                          //       potential messaging up to the app.
                          ActionFailure();
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
  // If we've somehow wiggled off the charge contacts, try the backup
  // onto charger action again to get us back onto the contacts
  if (!GetBEI().GetRobotInfo().IsOnChargerContacts()) {
    DelegateIfInControl(new BackupOntoChargerAction(_dVars.chargerID, true));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::ActionFailure()
{
  PRINT_NAMED_WARNING("BehaviorGoHome.ActionFailure",
                      "BehaviorGoHome ending due to an action failure");
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
