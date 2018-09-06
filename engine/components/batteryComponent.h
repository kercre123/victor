/**
 * File: batteryComponent.h
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

#ifndef __Engine_Components_BatteryComponent_H__
#define __Engine_Components_BatteryComponent_H__

#include "engine/robotComponents_fwd.h"

#include "clad/types/batteryTypes.h"

#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/common/shared/radians.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <cmath>

namespace Anki {
namespace Util {
  class LowPassFilterSimple;
}
namespace Vector {

class BlockWorldFilter;
class Robot;
struct RobotState;
  
namespace external_interface {
  class BatteryStateRequest;
  class GatewayWrapper;
}

class BatteryComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  BatteryComponent();
  ~BatteryComponent() = default;

  // IDependencyManagedComponent functions
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::Movement);
    dependencies.insert(RobotComponentID::FullRobotPose);
  };
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override {
    Init(robot);
  };
  // end IDependencyManagedComponent functions
  
  // Updates things when a RobotState message is received from the robot
  void NotifyOfRobotState(const RobotState& msg);
  
  BatteryLevel GetBatteryLevel() const { return _batteryLevel; }
  
  // Returns the low-pass filtered battery voltage
  float GetBatteryVolts() const { return _batteryVoltsFilt; }
  
  // Returns raw battery voltage as reported by the robot.
  // Note that this will be quite noisy and not super useful
  // for most use cases.
  float GetBatteryVoltsRaw() const { return _batteryVoltsRaw; }
  
  // Returns raw charger voltage as reported by the robot
  float GetChargerVoltsRaw() const { return _chargerVoltsRaw; }
  
  // Low battery - should charge as soon as possible
  bool IsBatteryLow() const { return _batteryLevel == BatteryLevel::Low; }
  
  // Battery is fully charged. Note that the battery will continue to charge
  // beyond this point, but at this point the robot is charged enough to be
  // able to wander off the charger and do stuff.
  bool IsBatteryFull() const { return _batteryLevel == BatteryLevel::Full; }
  
  // If the robot has been on the charger for more than 30 min, the battery
  // is disconnected from the charging circuit to prevent repeated trickle
  // discharge/charge cycles that reduce battery expectancy. 
  // It may also be disconnected if the battery overheats to 45C until it cools
  // down to 42C at which point it will be reconnected. The cumulative total
  // time that the battery is connected while on the charger should still be 30 min.
  // (Currently, when disconnected, GetBatteryVoltsRaw() returns a very low (~0.1V)
  // reading while GetBatteryVolts() holds the last valid filtered voltage reading, 
  // but this can be changed as needed.)
  bool IsBatteryDisconnectedFromCharger() const { return _battDisconnected; }

  // Indicates that the robot has its charge circuit enabled. Note that
  // this will remain true even after the battery is fully charged.
  bool IsCharging() const { return _isCharging; }
  
  // Indicates that the robot is sensing voltage on its charge contacts
  bool IsOnChargerContacts() const { return _isOnChargerContacts; }
  
  // True if we think the robot is on a charger. This becomes true only when the robot touches the charger
  // contacts, and remains true until we think the robot has driven off the charger. It will not become true
  // based on localization or observing the charger marker, only based on feeling the charger. A robot on the
  // charger contacts is always on the platform (NOTE: even if it thinks it's in the air or on it's side)
  bool IsOnChargerPlatform() const { return _isOnChargerPlatform; }
  
  // Returns whether or not the battery is overheated.
  // A power shutdown is imminent 30 seconds from when this first becomes true.
  bool IsBatteryOverheated() const { return _battOverheated; }

  // Returns how long the "fully charged" state has been active. Returns 0
  // if not currently fully charged.
  float GetFullyChargedTimeSec() const;
  
  // Returns how long the "low battery" state has been active. Returns 0
  // if not currently in a low battery state.
  float GetLowBatteryTimeSec() const;

  external_interface::GatewayWrapper GetBatteryState(const external_interface::BatteryStateRequest& request);
  
  uint8_t GetBatteryTemperature_C() const { return _battTemperature_C; }

  // If the robot is low battery, this will give a timer of how long it should charge for in seconds. Once elapsed,
  // or if the robot was not recently low battery, it will return 0.0
  float GetSuggestedChargerTime() const;

private:
  
  void Init(Robot* _robot);
  
  void SetOnChargeContacts(const bool onChargeContacts);
  void SetIsCharging(const bool isCharging);
  void UpdateOnChargerPlatform();
  void UpdateSuggestedChargerTime(bool wasLowBattery, bool wasCharging);
  
  // Filtered and raw battery voltage in mV
  int GetBatteryVolts_mV() { return static_cast<int>(std::round(1000.f * _batteryVoltsFilt)); }
  int GetBatteryVoltsRaw_mV() { return static_cast<int>(std::round(1000.f * _batteryVoltsRaw)); }
  
  Robot* _robot = nullptr;
  
  float _batteryVoltsRaw = 0.f;
  float _batteryVoltsFilt = 0.f;
  float _chargerVoltsRaw = 0.f;

  uint8_t _battTemperature_C = 0;

  BatteryLevel _batteryLevel = BatteryLevel::Unknown;
  
  bool _battOverheated = false;
  bool _battDisconnected = false;
  bool _isCharging = false;
  bool _isOnChargerContacts = false;
  bool _isOnChargerPlatform = false;
  bool _hasStoppedChargingSinceLastSaturationCharge = false;
  
  float _lastBatteryLevelChange_sec = 0;
  float _lastOnChargerContactsChange_sec = 0;
  
  float _saturationChargingStartTime_sec = 0.f;
  float _saturationChargeTimeRemaining_sec = 0.f;
  float _lastSaturationChargingEndTime_sec = 0.f;

  // The timestamp of the RobotState message with the latest data
  RobotTimeStamp_t _lastMsgTimestamp = 0;
  
  // The pitch angle of the robot when it was last on the charger
  // contacts (and not moving).
  Radians _lastOnChargerContactsPitchAngle;
  
  std::unique_ptr<BlockWorldFilter> _chargerFilter;
  
  std::unique_ptr<Util::LowPassFilterSimple> _batteryVoltsFilter;
  
  float _timeRemovedFromCharger_s = 0.f;
  float _suggestedChargeEndTime_s = 0.f;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_BatteryComponent_H__
