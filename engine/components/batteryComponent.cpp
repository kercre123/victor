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
  , _chargerFilter(std::make_unique<BlockWorldFilter>())
{
  // setup block world filter to find chargers:
  _chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  _chargerFilter->AddAllowedType(ObjectType::Charger_Basic);
}

void BatteryComponent::Init(Cozmo::Robot* robot)
{
  _robot = robot;
}

void BatteryComponent::NotifyOfRobotState(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;
  
  _rawBatteryVolts = msg.batteryVoltage;
  
  SetOnChargeContacts(msg.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER);
  SetIsCharging(msg.status & (uint32_t)RobotStatusFlag::IS_CHARGING);
}

void BatteryComponent::SetOnChargeContacts(const bool onChargeContacts)
{
  // If we are being set on a charger, we can update the instance of the charger in the current world to
  // match the robot. If we don't have an instance, we can add an instance now
  if (onChargeContacts)
  {
    const Pose3d& poseWrtRobot = Charger::GetDockPoseRelativeToRobot(*_robot);
    
    // find instance in current origin
    ObservableObject* chargerInstance = _robot->GetBlockWorld().FindLocatedMatchingObject(*_chargerFilter);
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
  
  // Log events and send message if state changed
  if (onChargeContacts != _isOnChargerContacts) {
    _isOnChargerContacts = onChargeContacts;
    if (onChargeContacts) {
      LOG_EVENT("robot.on_charger", "");
      // if we are on the charger, we must also be on the charger platform.
      SetOnChargerPlatform(true);
    } else {
      LOG_EVENT("robot.off_charger", "");
    }
    // Broadcast to game
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(ChargerEvent(onChargeContacts)));
  }
}


void BatteryComponent::SetOnChargerPlatform(bool onPlatform)
{
  // Can only not be on platform if not on charge contacts
  onPlatform = onPlatform || IsOnChargerContacts();
  
  const bool stateChanged = _isOnChargerPlatform != onPlatform;
  _isOnChargerPlatform = onPlatform;
  
  if (stateChanged) {
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(RobotOnChargerPlatformEvent(_isOnChargerPlatform)));
    
    // pause the freeplay tracking if we are on the charger
    _robot->GetAIComponent().GetComponent<FreeplayDataTracker>().SetFreeplayPauseFlag(_isOnChargerPlatform, FreeplayPauseFlag::OnCharger);
  }
}


void BatteryComponent::SetIsCharging(const bool isCharging)
{
  if (isCharging != _isCharging) {
    _lastChargingChange_ms = _lastMsgTimestamp;
    _isCharging = isCharging;
  }
}


} // Cozmo namespace
} // Anki namespace
