/**
 * File: batteryComponent.cpp
 *
 * Author: Matt Michini
 * Created: 2/27/2018
 *
 * Description: Component for monitoring battery state-of-charge, time on charger,
 *              and related information.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/batteryComponent.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/freeplayDataTracker.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/robot.h"

#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Cozmo {
  
BatteryComponent::BatteryComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Battery)
{
}

void BatteryComponent::Init(Cozmo::Robot* robot)
{
  _robot = robot;
}

void BatteryComponent::NotifyOfRobotState(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;
  
  _rawBatteryVolts = msg.batteryVoltage;
  
  SetOnCharger(msg.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER);
  SetIsCharging(msg.status & (uint32_t)RobotStatusFlag::IS_CHARGING);
}

void BatteryComponent::SetOnCharger(const bool onCharger)
{
  // If we are being set on a charger, we can update the instance of the charger in the current world to
  // match the robot. If we don't have an instance, we can add an instance now
  if (onCharger)
  {
    const Pose3d& poseWrtRobot = Charger::GetDockPoseRelativeToRobot(*_robot);
    
    // find instance in current origin
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::Charger);
    filter.AddAllowedType(ObjectType::Charger_Basic);
    ObservableObject* chargerInstance = _robot->GetBlockWorld().FindLocatedMatchingObject(filter);
    if (nullptr == chargerInstance)
    {
      // there's currently no located instance, we need to create one.
      chargerInstance = new Charger();
      chargerInstance->SetID();
    }
    
    // pretend the instance we created was an observation. Note that lastObservedTime will be 0 in this case, since
    // that timestamp refers to visual observations only (TODO: maybe that should be more explicit or any
    // observation should set that timestamp)
    _robot->GetObjectPoseConfirmer().AddRobotRelativeObservation(chargerInstance, poseWrtRobot, PoseState::Known);
  }
  
  // log events when onCharger status changes
  if (onCharger && !_isOnCharger)
  {
    // if we are on the charger, we must also be on the charger platform.
    SetOnChargerPlatform(true);
    
    // offCharger -> onCharger
    LOG_EVENT("robot.on_charger", "");
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(true)));
  }
  else if (!onCharger && _isOnCharger)
  {
    // onCharger -> offCharger
    LOG_EVENT("robot.off_charger", "");
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(false)));
  }
  
  // update flag now (note this gets updated after notifying; this might be an issue for listeners)
  _isOnCharger = onCharger;
}


void BatteryComponent::SetOnChargerPlatform(bool onPlatform)
{
  // Can only not be on platform if not on charge contacts
  onPlatform = onPlatform || IsOnCharger();
  
  const bool stateChanged = _isOnChargerPlatform != onPlatform;
  _isOnChargerPlatform = onPlatform;
  
  if (stateChanged) {
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotOnChargerPlatformEvent(_isOnChargerPlatform)));
    
    // pause the freeplay tracking if we are on the charger
    _robot->GetAIComponent().GetComponent<FreeplayDataTracker>().SetFreeplayPauseFlag(_isOnChargerPlatform, FreeplayPauseFlag::OnCharger);
  }
}


void BatteryComponent::SetIsCharging(bool isCharging)
{
  if (isCharging != _isCharging) {
    _lastChargingChange_ms = _lastMsgTimestamp;
    _isCharging = isCharging;
  }
}


} // Cozmo namespace
} // Anki namespace
