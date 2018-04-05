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
#include "engine/drivingAnimationHandler.h"
#include "engine/events/animationTriggerHelpers.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kUseCliffSensorsKey = "useCliffSensorCorrection";
const char* kLeftTurnAnimKey    = "leftTurnAnimTrigger";
const char* kRightTurnAnimKey   = "rightTurnAnimTrigger";
const char* kBackupStartAnimKey = "backupStartAnimTrigger";
const char* kBackupEndAnimKey   = "backupEndAnimTrigger";
const char* kBackupLoopAnimKey  = "backupLoopAnimTrigger";
const char* kRaiseLiftAnimKey   = "raiseLiftAnimTrigger";
const char* kNuzzleAnimKey      = "nuzzleAnimTrigger";
}
  
  
BehaviorGoHome::BehaviorGoHome(const Json::Value& config)
  : ICozmoBehavior(config)
  , _homeFilter(std::make_unique<BlockWorldFilter>())
{
  LoadConfig(config["params"]);
  
  // Set up block world filter for finding Home object
  _homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
 

bool BehaviorGoHome::WantsToBeActivatedBehavior() const
{
  const bool isOnCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( isOnCharger ) {
    return false;
  }
  
  // If we have a located Home/charger, then we can be activated.
  
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  return hasAHome;
}


void BehaviorGoHome::OnBehaviorActivated()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  // Push driving animations
  PushDrivingAnims();
  
  _chargerID = object->GetID();
  
  auto driveToAction = new DriveToObjectAction(_chargerID, PreActionPose::ActionType::DOCKING);
  driveToAction->SetPreActionPoseAngleTolerance(DEG_TO_RAD(15.f));
  DelegateIfInControl(driveToAction, &BehaviorGoHome::TransitionToTurn);
}


void BehaviorGoHome::OnBehaviorDeactivated()
{
  PopDrivingAnims();
}


void BehaviorGoHome::TransitionToTurn()
{
  // Turn to align with the charger
  DelegateIfInControl(new TurnToAlignWithChargerAction(_chargerID,
                                                       _params.leftTurnAnimTrigger,
                                                       _params.rightTurnAnimTrigger),
                      &BehaviorGoHome::TransitionToMountCharger);
}


void BehaviorGoHome::TransitionToMountCharger()
{
  // Play the animations to raise lift, then mount the charger
  auto* action = new CompoundActionSequential();
  action->AddAction(new TriggerAnimationAction(_params.raiseLiftAnimTrigger));
  action->AddAction(new MountChargerAction(_chargerID, _params.useCliffSensorCorrection));
  
  DelegateIfInControl(action, &BehaviorGoHome::TransitionToPlayingNuzzleAnim);
}


void BehaviorGoHome::TransitionToPlayingNuzzleAnim()
{
  // Remove driving animations
  PopDrivingAnims();
  
  DelegateIfInControl(new TriggerAnimationAction(_params.nuzzleAnimTrigger),
                      &BehaviorGoHome::TransitionToOnChargerCheck);
}


void BehaviorGoHome::TransitionToOnChargerCheck()
{
  // If we've somehow wiggled off the charge contacts, try the backup
  // onto charger action again to get us back onto the contacts
  if (!GetBEI().GetRobotInfo().IsOnCharger()) {
    DelegateIfInControl(new BackupOntoChargerAction(_chargerID, true));
  }
}
  
  
void BehaviorGoHome::LoadConfig(const Json::Value& config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  
  _params.useCliffSensorCorrection = JsonTools::ParseBool(config, kUseCliffSensorsKey, debugName);
  _params.leftTurnAnimTrigger      = JsonTools::ParseAnimationTrigger(config, kLeftTurnAnimKey, debugName);
  _params.rightTurnAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kRightTurnAnimKey, debugName);
  _params.backupStartAnimTrigger   = JsonTools::ParseAnimationTrigger(config, kBackupStartAnimKey, debugName);
  _params.backupEndAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kBackupEndAnimKey, debugName);
  _params.backupLoopAnimTrigger    = JsonTools::ParseAnimationTrigger(config, kBackupLoopAnimKey, debugName);
  _params.raiseLiftAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kRaiseLiftAnimKey, debugName);
  _params.nuzzleAnimTrigger        = JsonTools::ParseAnimationTrigger(config, kNuzzleAnimKey, debugName);
}


void BehaviorGoHome::PushDrivingAnims()
{
  DEV_ASSERT(!_drivingAnimsPushed, "BehaviorGoHome.PushDrivingAnims.AnimsAlreadyPushed");
  
  if (!_drivingAnimsPushed) {
    auto& drivingAnimHandler = GetBEI().GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.PushDrivingAnimations({_params.backupStartAnimTrigger,
                                              _params.backupLoopAnimTrigger,
                                              _params.backupEndAnimTrigger},
                                             GetDebugLabel());
    _drivingAnimsPushed = true;
  }
}


void BehaviorGoHome::PopDrivingAnims()
{
  if (_drivingAnimsPushed) {
    auto& drivingAnimHandler = GetBEI().GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.RemoveDrivingAnimations(GetDebugLabel());
    _drivingAnimsPushed = false;
  }
}

} // namespace Cozmo
} // namespace Anki
