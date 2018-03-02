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

#include "coretech/common/shared/types.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class Robot;
struct RobotState;

class BatteryComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  BatteryComponent();
  ~BatteryComponent() = default;

  // IDependencyManagedComponent functions
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override {
    Init(robot);
  };
  // end IDependencyManagedComponent functions
  
  void NotifyOfRobotState(const RobotState& msg);
  
  float GetRawBatteryVolts() const { return _rawBatteryVolts; }
  
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
  
  // Return the message timestamp of the last time the value of IsCharging changed
  TimeStamp_t GetLastChargingStateChangeTimestamp() const { return _lastChargingChange_ms; }
  
  void SetOnChargerPlatform(bool onPlatform);
  
private:
  
  void Init(Robot* _robot);
  
  void SetOnChargeContacts(const bool onChargeContacts);
  void SetIsCharging(const bool isCharging);
  
  Robot* _robot = nullptr;
  
  float _rawBatteryVolts = 0.f;

  bool _isCharging = false;
  bool _isOnChargerContacts = false;
  bool _isOnChargerPlatform = false;
  
  TimeStamp_t _lastChargingChange_ms = 0;
  
  // The timestamp of the RobotState message with the latest data
  TimeStamp_t _lastMsgTimestamp = 0;
  
  std::unique_ptr<BlockWorldFilter> _chargerFilter;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_BatteryComponent_H__
