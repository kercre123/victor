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
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorRollBlock.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRequestToGoHome.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {

namespace {
const char* kMaxNumAttemptsKey = "maxNumAttempts";
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClearChargerArea::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
  : maxNumAttempts(JsonTools::ParseInt32(config, kMaxNumAttemptsKey, debugName))
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
    kMaxNumAttemptsKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.pickupBehavior.get());
  delegates.insert(_iConfig.rollBehavior.get());
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
  
  BC.FindBehaviorByIDAndDowncast<BehaviorRollBlock>(BEHAVIOR_ID(PlayRollBlock),
                                                    BEHAVIOR_CLASS(RollBlock),
                                                    _iConfig.rollBehavior);
  DEV_ASSERT(_iConfig.rollBehavior != nullptr,
             "BehaviorClearChargerArea.InitBehavior.NullRollBehavior");
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
  
  // Attempt to pick up the cube
  const auto& cubeId = cube->GetID();
  _iConfig.pickupBehavior->SetTargetID(cubeId);
  const bool pickupWantsToRun = _iConfig.pickupBehavior->WantsToBeActivated();
  if (pickupWantsToRun) {
    DelegateIfInControl(_iConfig.pickupBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            // Succeeded in picking up cube
                            TransitionToPlacingCubeOnGround();
                          } else {
                            // We're not carrying a cube for some reason - try rolling the cube
                            TransitionToRollCube();
                          }
                        });
  } else {
    // Pickup does not want to run for some reason. Try rolling.
    TransitionToRollCube();
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToRollCube()
{
  const auto* cube = GetCubeInChargerArea();
  if (cube == nullptr) {
    // Cube no longer interfering. Yay!
    return;
  }
  
  // Attempt to roll the cube
  const auto& cubeId = cube->GetID();
  _iConfig.rollBehavior->SetTargetID(cubeId);
  const bool rollWantsToRun = _iConfig.rollBehavior->WantsToBeActivated();
  if (rollWantsToRun) {
    DelegateIfInControl(_iConfig.rollBehavior.get(),
                        &BehaviorClearChargerArea::TransitionToCheckDockingArea);
  } else {
    PRINT_NAMED_WARNING("BehaviorClearChargerArea.TransitionToRollCube.RollDoesNotWantToRun",
                        "Roll behavior does not want to run - falling back to charger request behavior");
    const bool requestWantsToRun = _iConfig.requestHomeBehavior->WantsToBeActivated();
    if (requestWantsToRun) {
      DelegateIfInControl(_iConfig.requestHomeBehavior.get());
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorClearChargerArea::TransitionToPlacingCubeOnGround()
{
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorClearChargerArea.TransitionToPlacingCubeOnGround.NullCharger", "Null charger!");
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
