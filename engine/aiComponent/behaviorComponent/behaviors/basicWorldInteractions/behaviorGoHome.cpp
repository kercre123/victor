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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/events/animationTriggerHelpers.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kUseCliffSensorsKey = "useCliffSensorCorrection";
const char* kTurningAnimKey = "turningAnimTrigger";
const char* kBackupAnimKey = "backupAnimTrigger";
const char* kNuzzleAnimKey = "nuzzleAnimTrigger";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::BehaviorGoHome(const Json::Value& config)
  : ICozmoBehavior(config)
  , _homeFilter(std::make_unique<BlockWorldFilter>())
{
  LoadConfig(config["params"]);
  
  // Set up block world filter for finding Home object
  _homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGoHome::WantsToBeActivatedBehavior() const
{
  // If we have a located Home/charger, then we can be activated.
  
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  return hasAHome;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorActivated()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  _chargerID = object->GetID();
  
  auto driveToAction = new DriveToObjectAction(_chargerID, PreActionPose::ActionType::DOCKING);
  driveToAction->SetPreActionPoseAngleTolerance(DEG_TO_RAD(15.f));
  DelegateIfInControl(driveToAction, &BehaviorGoHome::TransitionToTurn);
}


void BehaviorGoHome::TransitionToTurn()
{
  // Turn to align with the charger while playing the turning animation
  auto* action = new CompoundActionParallel();
  action->AddAction(new TurnToAlignWithChargerAction(_chargerID));
  action->AddAction(new TriggerAnimationAction(_params.turningAnimTrigger));
  
  DelegateIfInControl(action, &BehaviorGoHome::TransitionToMountCharger);
}


void BehaviorGoHome::TransitionToMountCharger()
{
  // Mount the charger while playing the backup animation
  auto* action = new CompoundActionParallel();
  action->AddAction(new MountChargerAction(_chargerID, _params.useCliffSensorCorrection));
  action->AddAction(new TriggerAnimationAction(_params.backupAnimTrigger));
  
  DelegateIfInControl(action, &BehaviorGoHome::TransitionToPlayingNuzzleAnim);
}


void BehaviorGoHome::TransitionToPlayingNuzzleAnim()
{
  DelegateIfInControl(new TriggerAnimationAction(_params.nuzzleAnimTrigger));
}

  
void BehaviorGoHome::LoadConfig(const Json::Value& config)
{
  const std::string& debugName = "Behavior" + GetIDStr() + ".LoadConfig";
  
  _params.useCliffSensorCorrection = JsonTools::ParseBool(config, kUseCliffSensorsKey, debugName);
  
  _params.turningAnimTrigger = JsonTools::ParseAnimationTrigger(config, kTurningAnimKey, debugName);
  
  _params.backupAnimTrigger = JsonTools::ParseAnimationTrigger(config, kBackupAnimKey, debugName);
  
  _params.nuzzleAnimTrigger = JsonTools::ParseAnimationTrigger(config, kNuzzleAnimKey, debugName);
}


} // namespace Cozmo
} // namespace Anki
