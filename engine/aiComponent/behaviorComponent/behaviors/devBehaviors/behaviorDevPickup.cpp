/**
 * File: BehaviorDevPickup.cpp
 *
 * Author: Matt Michini
 * Created: 2018-08-29
 *
 * Description: testing a new pickup cube method
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevPickup.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/navMap/mapComponent.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/types/behaviorComponent/behaviorStats.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevPickup::InstanceConfig::InstanceConfig()
  : cubesFilter(std::make_unique<BlockWorldFilter>())
{
  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevPickup::BehaviorDevPickup(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevPickup::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPickup::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPickup::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
 
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  _dVars.cubePtr = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.cubesFilter);
  
  if (_dVars.cubePtr == nullptr) {
    PRINT_NAMED_WARNING("BehaviorDevPickup.OnBehaviorActivated.NoCubeFound", "No cube in blockworld - exiting");
    return;
  }
  
  // Overall sequence of actions for this behavior:
  // 1. Drive to the cube's normal pre-dock pose.
  // 2. Re-observe the cube.
  // 3. Drive to a pose that is ~50 mm directly in front of the nearest marker, and facing directly at the marker.
  // 4. Re-observe the cube. If not seen, raise lift and try again.
  // 5. Turn toward the center of the marker.
  // 6. Drive forward a bit to engage the lift with the cube.
  // 7. (maybe) ensure that the marker is still observed.
  // 8. Raise the lift and ensure we've picked up the cube.

  auto* action = new CompoundActionSequential();
  action->AddAction(new DriveToObjectAction{_dVars.cubePtr->GetID(), PreActionPose::ActionType::DOCKING});
  action->AddAction(new VisuallyVerifyObjectAction{_dVars.cubePtr->GetID()});
  
  DelegateIfInControl(action,
                      &BehaviorDevPickup::TransitionToDriveToInFrontOfCube);
}


void BehaviorDevPickup::TransitionToDriveToInFrontOfCube()
{
  const float distToCube_mm = 70.f;
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new DriveToObjectAction{_dVars.cubePtr->GetID(), distToCube_mm});
  action->AddAction(new VisuallyVerifyObjectAction{_dVars.cubePtr->GetID()});
  
  DelegateIfInControl(action,
                      &BehaviorDevPickup::TransitionToFineTurn);
}

void BehaviorDevPickup::TransitionToFineTurn()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  
  Pose3d markerPose;
  
  const bool ignoreZ = true;
  const auto result = _dVars.cubePtr->GetClosestMarkerPose(robotPose, ignoreZ, markerPose);
  
  if (result != RESULT_OK) {
    PRINT_NAMED_WARNING("BehaviorDevPickup.TransitionToFineTurn.FailedGettingClosestMarkerPose", "");
    return;
  }
  
  const float driveDist_mm = 30.f;
  
  auto* action = new CompoundActionSequential();
  
  auto* turnToPose = new TurnTowardsPoseAction{markerPose};
  turnToPose->SetPanTolerance(DEG_TO_RAD(2.f));
  
  action->AddAction(turnToPose);
  action->AddAction(new DriveStraightAction{driveDist_mm});
  action->AddAction(new MoveLiftToHeightAction{MoveLiftToHeightAction::Preset::HIGH_DOCK});
  
  DelegateIfInControl(action);
}
  
  
}
}
