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
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::InstanceConfig::InstanceConfig()
{
  useCliffSensorCorrection = true;
  leftTurnAnimTrigger      = AnimationTrigger::Count;
  rightTurnAnimTrigger     = AnimationTrigger::Count;
  backupStartAnimTrigger   = AnimationTrigger::Count;
  backupEndAnimTrigger     = AnimationTrigger::Count;
  backupLoopAnimTrigger    = AnimationTrigger::Count;
  raiseLiftAnimTrigger     = AnimationTrigger::Count;
  nuzzleAnimTrigger        = AnimationTrigger::Count;
  homeFilter               = std::make_unique<BlockWorldFilter>();

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{  
  useCliffSensorCorrection = JsonTools::ParseBool(config, kUseCliffSensorsKey, debugName);
  leftTurnAnimTrigger      = JsonTools::ParseAnimationTrigger(config, kLeftTurnAnimKey, debugName);
  rightTurnAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kRightTurnAnimKey, debugName);
  backupStartAnimTrigger   = JsonTools::ParseAnimationTrigger(config, kBackupStartAnimKey, debugName);
  backupEndAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kBackupEndAnimKey, debugName);
  backupLoopAnimTrigger    = JsonTools::ParseAnimationTrigger(config, kBackupLoopAnimKey, debugName);
  raiseLiftAnimTrigger     = JsonTools::ParseAnimationTrigger(config, kRaiseLiftAnimKey, debugName);
  nuzzleAnimTrigger        = JsonTools::ParseAnimationTrigger(config, kNuzzleAnimKey, debugName);
  homeFilter               = std::make_unique<BlockWorldFilter>();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::DynamicVariables::DynamicVariables()
{
  drivingAnimsPushed = false;
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
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGoHome::WantsToBeActivatedBehavior() const
{
  const bool isOnCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( isOnCharger ) {
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
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_ERROR("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  // Push driving animations
  PushDrivingAnims();
  
  _dVars.chargerID = object->GetID();
  
  auto driveToAction = new DriveToObjectAction(_dVars.chargerID, PreActionPose::ActionType::DOCKING);
  driveToAction->SetPreActionPoseAngleTolerance(DEG_TO_RAD(15.f));
  DelegateIfInControl(driveToAction,
                      [this](ActionResult result) {
                        if (result == ActionResult::SUCCESS) {
                          TransitionToTurn();
                        } else {
                          ActionFailure();
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorDeactivated()
{
  PopDrivingAnims();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::TransitionToTurn()
{
  // Turn to align with the charger
  DelegateIfInControl(new TurnToAlignWithChargerAction(_dVars.chargerID,
                                                       _iConfig.leftTurnAnimTrigger,
                                                       _iConfig.rightTurnAnimTrigger),
                      [this](ActionResult result) {
                        if (result == ActionResult::SUCCESS) {
                          TransitionToMountCharger();
                        } else {
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
                        if (result == ActionResult::SUCCESS) {
                          TransitionToPlayingNuzzleAnim();
                        } else {
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

} // namespace Cozmo
} // namespace Anki
