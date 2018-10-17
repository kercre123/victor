/**
 * File: BehaviorClearChargerArea
 *
 * Author: Matt Michini
 * Created: 5/9/18
 *
 * Description: One way or another, clear out the area in front of the charger so the robot
 *              successfully docks with it.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorClearChargerArea.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRequestToGoHome.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/rotatedRect.h"

#define LOG_FUNCTION_NAME() PRINT_CH_INFO("Behaviors", "BehaviorClearChargerArea", "BehaviorClearChargerArea.%s", __func__);

namespace Anki {
namespace Vector {

namespace {
  const char* kTryToPickUpCubeKey = "tryToPickUpCube";
  const char* kMaxNumAttemptsKey  = "maxNumAttempts";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClearChargerArea::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
  : maxNumAttempts(JsonTools::ParseInt32(config, kMaxNumAttemptsKey, debugName))
  , tryToPickUpCube(JsonTools::ParseBool(config, kTryToPickUpCubeKey, debugName))
  , chargerFilter(std::make_unique<BlockWorldFilter>())
  , cubesFilter(std::make_unique<BlockWorldFilter>())
{ 
  DEV_ASSERT(maxNumAttempts > 0, "BehaviorClearChargerArea.InstanceConfig.InvalidMaxNumAttempts");
  
  chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);
  
  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClearChargerArea::BehaviorClearChargerArea(const Json::Value& config)
  : ICozmoBehavior(config)
  , _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTryToPickUpCubeKey,
    kMaxNumAttemptsKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.pickupBehavior.get());
  delegates.insert(_iConfig.requestHomeBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(BEHAVIOR_ID(PickupCube),
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickupBehavior);
  DEV_ASSERT(_iConfig.pickupBehavior != nullptr,
             "BehaviorClearChargerArea.InitBehavior.NullPickupBehavior");
  
  BC.FindBehaviorByIDAndDowncast<BehaviorRequestToGoHome>(BEHAVIOR_ID(RequestHomeBecauseStuck),
                                                          BEHAVIOR_CLASS(RequestToGoHome),
                                                          _iConfig.requestHomeBehavior);
  DEV_ASSERT(_iConfig.requestHomeBehavior != nullptr,
             "BehaviorClearChargerArea.InitBehavior.NullRequestHomeBehavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorClearChargerArea::WantsToBeActivatedBehavior() const
{
  // If we have a located Home/charger, and there is a cube in its
  // docking area, then we can be activated.
  const bool cubeInChargerArea = (GetCubeInChargerArea() != nullptr);
  return cubeInChargerArea;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::OnBehaviorActivated()
{
  LOG_FUNCTION_NAME();
  
  _dVars = DynamicVariables();

  const auto* cube = GetCubeInChargerArea();
  if (cube == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.OnBehaviorActivated.NoCubeInChargerArea",
                      "There is no cube intersecting the charger area!");
    return;
  }
  
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.chargerFilter);
  
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.OnBehaviorActivated", "No charger found!");
    return;
  }
  
  _dVars.chargerID = charger->GetID();
  
  // Check here if we're carrying an object and put it down next to the charger if so
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    TransitionToPlacingCubeOnGround();
  } else {
    TransitionToCheckDockingArea();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToCheckDockingArea()
{
  LOG_FUNCTION_NAME();
  
  const auto* cube = GetCubeInChargerArea();
  if (cube == nullptr) {
    // Success! Nothing in the way.
    return;
  }
 
  if (++_dVars.numAttempts > _iConfig.maxNumAttempts) {
    PRINT_NAMED_WARNING("BehaviorClearChargerArea.TransitionToCheckDockingArea.OutOfRetries",
                        "We've attempted to clear the charger docking area the maximum number of times (%d) - falling back to charger request behavior",
                        _iConfig.maxNumAttempts);
    const bool requestWantsToRun = _iConfig.requestHomeBehavior->WantsToBeActivated();
    if (requestWantsToRun) {
      DelegateIfInControl(_iConfig.requestHomeBehavior.get());
    }
    return;
  }
  
  // Attempt to pick up the cube (if configured to do so)
  const auto& cubeId = cube->GetID();
  _iConfig.pickupBehavior->SetTargetID(cubeId);
  const bool pickupWantsToRun = _iConfig.pickupBehavior->WantsToBeActivated();
  if (_iConfig.tryToPickUpCube && pickupWantsToRun) {
    DelegateIfInControl(_iConfig.pickupBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            // Succeeded in picking up cube
                            TransitionToPlacingCubeOnGround();
                          } else {
                            // We're not carrying a cube for some reason - try shoving the cube
                            TransitionToPositionForRamming();
                          }
                        });
  } else {
    // Pickup does not want to run for some reason. Try shoving it.
    TransitionToPositionForRamming();
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToPositionForRamming()
{
  LOG_FUNCTION_NAME();
  
  const auto* cube = GetCubeInChargerArea();
  if (cube == nullptr) {
    // Cube no longer interfering. Yay!
    return;
  }
  
  // From where the robot currently is, see if we are able to shove the block out of the way (i.e. if we shove it from
  // our current position, it should not hit the charger). Do this by creating a stretched rectangle roughly the width
  // of the robot and extending it along the path from the robot to the cube (and past the cube), then check if this
  // rectangle intersects the charger.
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto robotX = robotPose.GetTranslation().x();
  const auto robotY = robotPose.GetTranslation().y();
  const auto& cubePose = cube->GetPose();
  const float cubeToRobotAngle = atan2f(cubePose.GetTranslation().y() - robotY,
                                        cubePose.GetTranslation().x() - robotX);
  const float cubeToRobotDist_mm = ComputeDistanceBetween(robotPose, cubePose);
  Point2f p1(robotX - 0.5f * ROBOT_BOUNDING_Y * std::sin(cubeToRobotAngle),
             robotY + 0.5f * ROBOT_BOUNDING_Y * std::cos(cubeToRobotAngle));
  Point2f p2(robotX + 0.5f * ROBOT_BOUNDING_Y * std::sin(cubeToRobotAngle),
             robotY - 0.5f * ROBOT_BOUNDING_Y * std::cos(cubeToRobotAngle));
  
  const float extDistance_mm = 1000.f;
  RotatedRectangle rect(p1.x(), p1.y(), p2.x(), p2.y(), extDistance_mm);
  
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.TransitionToPositionForRamming.NullCharger", "Null charger!");
    return;
  }
  
  auto* action = new CompoundActionSequential();
  
  // First move the lift down, so that the robot can effectively 'shove' it to the side
  action->AddAction(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK));
  
  const bool shovingPathIntersectsCharger = charger->GetBoundingQuadXY().Intersects(rect.GetQuad());
  if (shovingPathIntersectsCharger) {
    // Cannot ram the block, since we would ram it into the charger. Position ourselves to the side of the charger
    // first before shoving it. Choose the side such that we will shove the cube _away_ from the charger.
    const auto& chargerPose = charger->GetPose();
    const float chargerToRobotAngle = atan2f(chargerPose.GetTranslation().y() - robotY,
                                             chargerPose.GetTranslation().x() - robotX);
    const float direction = Radians(cubeToRobotAngle - chargerToRobotAngle) < 0.f ? 1.f : -1.f;
    const float lateralDist_mm = 65.f; // This puts the robot roughly right next to the cube
    const float turnAngle_rad = cubeToRobotAngle + direction * std::atan(lateralDist_mm / cubeToRobotDist_mm);
    const float driveDist_mm = std::abs(DRIVE_CENTER_OFFSET) + std::sqrt(cubeToRobotDist_mm * cubeToRobotDist_mm - lateralDist_mm * lateralDist_mm);
    
    action->AddAction(new TurnInPlaceAction(turnAngle_rad, true));
    action->AddAction(new DriveStraightAction(driveDist_mm));
    action->AddAction(new TurnInPlaceAction(-direction * M_PI_2_F, false));
  } else {
    // Just turn toward the cube before ramming it.
    action->AddAction(new TurnTowardsPoseAction(cubePose));
  }
  
  DelegateIfInControl(action,
                      &BehaviorClearChargerArea::TransitionToRamCube);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToRamCube()
{
  LOG_FUNCTION_NAME();
  
  const auto* cube = GetCubeInChargerArea();
  if (cube == nullptr) {
    // Cube no longer interfering. Yay!
    return;
  }
  
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.TransitionToRamCube.NullCharger", "Null charger!");
    return;
  }
  
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto& cubePose = cube->GetPose();
  const float cubeToRobotDist_mm = ComputeDistanceBetween(robotPose, cubePose);
  
  auto* action = new CompoundActionSequential();
  const float rammingDist_mm = 100.f + cubeToRobotDist_mm;
  const float backupDist_mm = -50.f;
  action->AddAction(new DriveStraightAction(rammingDist_mm));
  action->AddAction(new DriveStraightAction(backupDist_mm));
  action->AddAction(new VisuallyVerifyObjectAction(cube->GetID()), true); // ignore failure
  action->AddAction(new TurnTowardsObjectAction(charger->GetID()));
  DelegateIfInControl(action,
                      [this](){
                        // If we still think the block is in front of the charger, then remove it from blockworld.
                        // Ramming it will have caused its position estimate to get messed up, and we do not want the
                        // robot to continue to think that the cube is in front of the charger if it's really not.
                        const auto* cube = GetCubeInChargerArea();
                        if (cube != nullptr) {
                          BlockWorldFilter deleteFilter;
                          deleteFilter.AddAllowedID(cube->GetID());
                          GetBEI().GetBlockWorld().DeleteLocatedObjects(deleteFilter);
                        }
                        TransitionToCheckDockingArea();
                      });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToPlacingCubeOnGround()
{
  LOG_FUNCTION_NAME();
  
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.TransitionToPlacingCubeOnGround.NullCharger", "Null charger!");
    return;
  }
  
  // Place the cube on the ground next to the charger.
  // Generate some optional placement poses, and let DriveToPose decide which one is best.
  const auto& chargerPose = charger->GetPose();
  std::vector<Pose3d> cubePlacementPoses;
  cubePlacementPoses.push_back(Pose3d{DEG_TO_RAD( 40.f), Z_AXIS_3D(), {-35.f,  60.f, 0.f}, chargerPose});
  cubePlacementPoses.push_back(Pose3d{DEG_TO_RAD( 50.f), Z_AXIS_3D(), {-50.f,  65.f, 0.f}, chargerPose});
  cubePlacementPoses.push_back(Pose3d{DEG_TO_RAD(-40.f), Z_AXIS_3D(), {-35.f, -60.f, 0.f}, chargerPose});
  cubePlacementPoses.push_back(Pose3d{DEG_TO_RAD(-50.f), Z_AXIS_3D(), {-50.f, -65.f, 0.f}, chargerPose});
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new DriveToPoseAction(cubePlacementPoses, false));
  action->AddAction(new PlaceObjectOnGroundAction());
  
  DelegateIfInControl(action,
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
                                              &BehaviorClearChargerArea::TransitionToCheckDockingArea);
                        } else {
                          TransitionToCheckDockingArea();
                        }
                      });
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::OnBehaviorDeactivated()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorClearChargerArea::GetCubeInChargerArea() const
{
  const auto& blockWorld = GetBEI().GetBlockWorld();

  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* charger = dynamic_cast<const Charger*>(blockWorld.FindLocatedObjectClosestTo(robotPose, *_iConfig.chargerFilter));
  if (charger == nullptr) {
    return nullptr;
  }
  
  std::vector<const ObservableObject*> intersectingCubes;
  blockWorld.FindLocatedIntersectingObjects(charger->GetDockingAreaQuad(),
                                            intersectingCubes,
                                            0.f,
                                            *_iConfig.cubesFilter);
  if (intersectingCubes.empty()) {
    return nullptr;
  }
  
  // Return the closest cube
  BlockWorldFilter intersectingCubesFilter;
  for (const auto& obj : intersectingCubes) {
    intersectingCubesFilter.AddAllowedID(obj->GetID());
  }
  return blockWorld.FindLocatedObjectClosestTo(robotPose,
                                               intersectingCubesFilter);
}


} // namespace Vector
} // namespace Anki
