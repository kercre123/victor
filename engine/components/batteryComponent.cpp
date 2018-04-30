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
  // after _saturationChargeTimeRemaining_sec expires.
  const float kSaturationChargingThresholdVolts = 4.1f;

  // Max time to wait after kSaturationChargingThresholdVolts is reached
  // before battery is considered "fully charged".
  const float kMaxSaturationTime_sec = 7 * 60.f;

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
  , _saturationChargeTimeRemaining_sec(kMaxSaturationTime_sec)
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
  bool wasDisconnected = _battDisconnected;
  _battDisconnected = (msg.status & (uint32_t)RobotStatusFlag::IS_BATTERY_DISCONNECTED)
                      || (_batteryVoltsRaw < 3);  // Just in case syscon doesn't report IS_BATTERY_DISCONNECTED for some reason.
                                                  // Anything under 3V doesn't make sense.
  if (!_battDisconnected) {
    _batteryVoltsFilt = _batteryVoltsFilter->AddSample(_batteryVoltsRaw);
  }

  // Update isCharging / isOnChargerContacts
  SetOnChargeContacts(msg.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER);
  SetIsCharging(msg.status & (uint32_t)RobotStatusFlag::IS_CHARGING);

  // Check if saturation charging
  bool isSaturationCharging = _isCharging && _batteryVoltsFilt > kSaturationChargingThresholdVolts;
  bool isFullyCharged = false;
  if (isSaturationCharging) {
    if (_saturationChargingStartTime_sec <= 0.f) {
      // Saturation charging has started
      _saturationChargingStartTime_sec = now_sec;

      // The amount of time until fully charged is the (discounted) amount of time
      // that has elapsed since the last time it was saturation charging
      // plus the amount of saturation charge time that was left when it ended
      // to a max time of kMaxSaturationTime_sec.
      //
      // kSaturationTimeReplenishmentSpeed represents a heuristic that very roughly
      // approximates the amount of saturation charge time required to offset the
      // amount of time spent since it was last saturation charging (which obviously
      // depends on exactly what it was doing off charger).
      //
      // NOTE: This replenish rate of 60% seems to work fine based on a test
      //       where the wheels were run full speed right after coming fully charged
      //       off the charger and returning to charger after 7 minutes. This resulted in
      //       just over 4 minutes of saturation charge time and seemed to yield as much
      //       subsequent discharge time as a fully charged battery.
      const float kSaturationTimeReplenishmentSpeed = 0.6f;  //  1: Replenish at real-time rate
                                                             // >1: Replenish faster (i.e. Takes longer to reach full)
                                                             // <1: Replenish slower (i.e. Faster to reach full)
      const float newPossibleSaturationChargeTimeRemaining_sec = _saturationChargeTimeRemaining_sec +
                                                                 (kSaturationTimeReplenishmentSpeed *
                                                                  (now_sec - _lastSaturationChargingEndTime_sec));
      _saturationChargeTimeRemaining_sec = std::min(kMaxSaturationTime_sec, newPossibleSaturationChargeTimeRemaining_sec);

      Util::sInfoF("BatteryComponent.NotifyOfRobotState.SaturationChargeStartVoltage",
                   {{DDATA, std::to_string(_batteryVoltsFilt).c_str()}}, "%f", _saturationChargeTimeRemaining_sec);
    }
    _lastSaturationChargingEndTime_sec = now_sec;

    isFullyCharged = now_sec > _saturationChargingStartTime_sec + _saturationChargeTimeRemaining_sec;

    // If transitioning to full, write DAS log
    if (isFullyCharged && !IsBatteryFull()) {
       Util::sInfoF("BatteryComponent.NotifyOfRobotState.FullyChargedVoltage",
                    {{DDATA, std::to_string(_batteryVoltsFilt).c_str()}}, "");
    }

  } else if (_saturationChargingStartTime_sec > 0.f) {
    // Saturation charging has stopped so update the amount of
    // saturation charge time remaining by subtracting the amount of
    // time that has elapsed since saturation charging started
    const float newPossibleSaturationChargeTimeRemaining_sec = _saturationChargeTimeRemaining_sec -
                                                               (now_sec - _saturationChargingStartTime_sec);
    _saturationChargeTimeRemaining_sec = std::max(0.f, newPossibleSaturationChargeTimeRemaining_sec);
    _saturationChargingStartTime_sec = 0.f;
  }

  // Battery should always be full when battery disconnects.
  // If it isn't, we may need to adjust kSaturationChargingThresholdVolts
  // or possibly the syscon cutoff time.
  // Current battery voltage should also be non-zero, otherwise it means
  // the engine started while the battery was already disconnected which
  // does not warrant a warning.
  if (_battDisconnected) {
    if (!wasDisconnected && !IsBatteryFull() && !NEAR_ZERO(_batteryVoltsFilt) ) {
      PRINT_NAMED_WARNING("BatteryComponent.NotifyOfRobotState.FullBatteryExpected", "%f", _batteryVoltsFilt);
    }

    // Force full battery state when disconnected.
    // It's not going to get anymore charged so might as well
    // pretend we're full.
    isFullyCharged = true;
  }

  // Update battery charge level
  BatteryLevel level = BatteryLevel::Nominal;
  if (isFullyCharged) {
    // NOTE: Given the dependence on isFullyCharged, this means BatteryLevel::Full is a state
    //       that can only be achieved while on charger
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
      PRINT_NAMED_INFO("robot.on_charger", "");
      // if we are on the charger, we must also be on the charger platform.
      SetOnChargerPlatform(true);
    } else {
      PRINT_NAMED_INFO("robot.off_charger", "");
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
