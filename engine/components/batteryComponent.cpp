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

#include <unistd.h>

namespace Anki {
namespace Cozmo {

namespace {
  // How often the filtered voltage reading is updated
  // (i.e. Rate of RobotState messages)
  const float kBatteryVoltsUpdatePeriod_sec =  STATE_MESSAGE_FREQUENCY * ROBOT_TIME_STEP_MS / 1000.f;
  
  // Time constant of the low-pass filter for battery voltage
  const float kBatteryVoltsFilterTimeConstant_sec = 6.0f;
  
  // Voltage above which the battery is considered fully charged
  const float kFullyChargedThresholdVolts = 4.1f;
  
  // Voltage below which battery is considered in a low charge state
  // At 3.6V, there is about 7 minutes of battery life left (if stationary, minimal processing, no wifi transmission, no sound)
  const float kLowBatteryThresholdVolts = 3.6f;

  // Approaching syscon cutoff voltage.
  // Shutdown will occur in ~30 seconds.
  const float kCriticalBatteryThresholdVolts = 3.45f;

  // How often to call sync() when battery is critical
  const float kCriticalBatterySyncPeriod_sec = 10.f;
  float _nextSyncTime_sec = 0;
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
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // Update raw voltage
  _batteryVoltsRaw = msg.batteryVoltage;

  // Only update filtered value if the battery isn't disconnected
  _battDisconnected = (msg.status & (uint32_t)RobotStatusFlag::IS_BATTERY_DISCONNECTED);
  if (!_battDisconnected) {
    _batteryVoltsFilt = _batteryVoltsFilter->AddSample(_batteryVoltsRaw);
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

    // Battery is critical 
    // Power shutdown is practically imminent so call
    // sync() every once in a while.
    if ((_batteryVoltsFilt < kCriticalBatteryThresholdVolts) &&
        (now_sec > _nextSyncTime_sec)) {
      sync();
      _nextSyncTime_sec = now_sec + kCriticalBatterySyncPeriod_sec;
    }
  }
  
  if (level != _batteryLevel) {
    PRINT_NAMED_INFO("BatteryComponent.BatteryLevelChanged",
                     "New battery level %s (previously %s for %f seconds)",
                     BatteryLevelToString(level),
                     BatteryLevelToString(_batteryLevel),
                     now_sec - _lastBatteryLevelChange_sec);
    _lastBatteryLevelChange_sec = now_sec;
    _batteryLevel = level;
  }
}


float BatteryComponent::GetFullyChargedTimeSec() const
{
  float timeSinceFullyCharged_sec = 0.f;
  if (_batteryLevel == BatteryLevel::Full) {
    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    timeSinceFullyCharged_sec = now_sec - _lastBatteryLevelChange_sec;
  }
  return timeSinceFullyCharged_sec;
}


float BatteryComponent::GetLowBatteryTimeSec() const
{
  float timeSinceLowBattery_sec = 0.f;
  if (_batteryLevel == BatteryLevel::Low) {
    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    timeSinceLowBattery_sec = now_sec - _lastBatteryLevelChange_sec;
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
