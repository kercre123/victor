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

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/types/robotStatusAndActions.h"

#include "osState/osState.h"

#include "util/filters/lowPassFilterSimple.h"

namespace Anki {
namespace Cozmo {

namespace {
  // How often we call into the system to get the current voltage.
  // This is expensive since it requires file access, so this rate
  // should be relatively slow.
  const float kBatteryVoltsUpdatePeriod_sec = 0.5f;
  
  // Time constant of the low-pass filter for battery voltage
  const float kBatteryVoltsFilterTimeConstant_sec = 6.0f;
  
  // Voltage above which the battery is considered fully charged
  const float kFullyChargedThresholdVolts = 4.0f;
  
  // Voltage below which battery is considered in a low charge state
  const float kLowBatteryThresholdVolts = 3.5f;
}

BatteryComponent::BatteryComponent()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Battery)
  , _chargerFilter(std::make_unique<BlockWorldFilter>())
{
  // setup battery voltage low pass filter (samples come in at kBatteryVoltsUpdatePeriod_sec)
  _batteryVoltsFilter = std::make_unique<Util::LowPassFilterSimple>(kBatteryVoltsUpdatePeriod_sec,
                                                                    kBatteryVoltsFilterTimeConstant_sec);
  
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
  
  // Poll the system voltage if it's the appropriate time
  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (now - _lastBatteryVoltsUpdate_sec > kBatteryVoltsUpdatePeriod_sec) {
    _batteryVoltsRaw = msg.batteryVoltage;
    _batteryVoltsFilt = _batteryVoltsFilter->AddSample(_batteryVoltsRaw);
    _lastBatteryVoltsUpdate_sec = now;
  }
  
  // Update isCharging / isOnChargerContacts
  SetOnChargeContacts(msg.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER);
  SetIsCharging(msg.status & (uint32_t)RobotStatusFlag::IS_CHARGING);
  
  // Update battery charge level
  BatteryLevel level = BatteryLevel::Nominal;
  if (_batteryVoltsFilt > kFullyChargedThresholdVolts) {
    level = BatteryLevel::Full;
  } else if (_batteryVoltsFilt < kLowBatteryThresholdVolts) {
    level = BatteryLevel::Low;
  }
  
  if (level != _batteryLevel) {
    const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    PRINT_NAMED_INFO("BatteryComponent.BatteryLevelChanged",
                     "New battery level %s (previously %s for %f seconds)",
                     BatteryLevelToString(level),
                     BatteryLevelToString(_batteryLevel),
                     now - _lastBatteryLevelChange_sec);
    _lastBatteryLevelChange_sec = now;
    _batteryLevel = level;
  }
}


float BatteryComponent::GetFullyChargedTimeSec() const
{
  float timeSinceFullyCharged_sec = 0.f;
  if (_batteryLevel == BatteryLevel::Full) {
    const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    timeSinceFullyCharged_sec = now - _lastBatteryLevelChange_sec;
  }
  return timeSinceFullyCharged_sec;
}


float BatteryComponent::GetLowBatteryTimeSec() const
{
  float timeSinceLowBattery_sec = 0.f;
  if (_batteryLevel == BatteryLevel::Low) {
    const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    timeSinceLowBattery_sec = now - _lastBatteryLevelChange_sec;
  }
  return timeSinceLowBattery_sec;
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
