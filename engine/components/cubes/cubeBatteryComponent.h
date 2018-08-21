/**
 * File: cubeBatteryComponent.h
 *
 * Author: Matt Michini
 * Created: 08/15/2018
 *
 * Description: Manages raw battery voltage messages from the cube and interfaces with external components.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_CubeBatteryComponent_H__
#define __Engine_Components_CubeBatteryComponent_H__

#include "engine/cozmoObservableObject.h"
#include "engine/robotComponents_fwd.h"

#include "coretech/common/shared/types.h"

#include "cubeBleClient/cubeBleClient.h" // alias BleFactoryId (should be moved to CLAD)

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <memory>

namespace Anki {
namespace Vector {

namespace external_interface {
  class CubeBattery;
}
  
struct CubeVoltageData;
class IGatewayInterface;
class Robot;

class CubeBatteryComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  CubeBatteryComponent();
  virtual ~CubeBatteryComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CubeComms);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  // Returns an allocated message containing the most recent/relevant cube battery information. If a cube is currently
  // connected, it will return battery information for that cube. Otherwise, it will return battery info (if available)
  // from the most recently heard-from cube. Otherwise, it will return nullptr.
  std::unique_ptr<external_interface::CubeBattery> GetCubeBatteryMsg();
  
  // Get the last reported battery voltage for the given cube. Returns -1 and prints a warning if we have not received
  // a voltage message from the given cube.
  float GetCubeBatteryVoltage(const BleFactoryId& factoryId);
  
  void HandleCubeVoltageData(const BleFactoryId& factoryId,
                             const CubeVoltageData& voltageData);
  
private:
  
  Robot* _robot = nullptr;
  
  IGatewayInterface* _gi = nullptr;

  struct CubeBatteryInfo {
    float batteryVolts = -1.f;
    float timeOfLastReading_sec = -1.f; // basestation timer
    int numConsecutiveLowReadings = 0;
  };
  
  std::map<BleFactoryId, CubeBatteryInfo> _cubeBatteryInfo;
  
};

}
}

#endif //__Engine_Components_CubeBatteryComponent_H__
