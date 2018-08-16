/**
 * File: cubeBatteryComponent.cpp
 *
 * Author: Matt Michini
 * Created: 08/15/2018
 *
 * Description: Manages raw battery voltage messages from the cube and interfaces with external components.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/cubes/cubeBatteryComponent.h"

#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"

#include "clad/externalInterface/messageCubeToEngine.h"

#include "coretech/common/engine/utils/timer.h"

#include "proto/external_interface/shared.pb.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
namespace {
  // According to VIC-5510, the conversion from raw ADC counts to volts goes according to the following:
  // volts = rawCnts * 3.6 / 1024
  constexpr float kBatteryRawCntsToVolts = 3.6f / 1024.f;
  
  // According to hardware team, this is a rough indication of when the cube is "low battery"
  const float kCubeLowBatteryThresh_volts = 1.1f;
}
  
CubeBatteryComponent::CubeBatteryComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeBattery)
{
}


void CubeBatteryComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  
  if( robot->HasGatewayInterface() ) {
    _gi = robot->GetGatewayInterface();
  }
}


void CubeBatteryComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
}


float CubeBatteryComponent::GetCubeBatteryVoltage(const BleFactoryId& factoryId)
{
  const auto cubeIt = _cubeBatteryVoltages.find(factoryId);
  if (cubeIt == _cubeBatteryVoltages.end()) {
    PRINT_NAMED_WARNING("CubeBatteryComponent.GetCubeBatteryVoltage.CubeNotFound",
                        "We have never received a battery voltage message from cube with factory ID %s",
                        factoryId.c_str());
    return -1.f;
  }
  return cubeIt->second;
}


void CubeBatteryComponent::HandleCubeVoltageData(const BleFactoryId& factoryId,
                                                 const CubeVoltageData& voltageData)
{
  // Convert from raw ADC counts to volts
  const float batteryVolts = voltageData.railVoltageCnts * kBatteryRawCntsToVolts;
  
  // Reject invalid readings
  if (!Util::InRange(batteryVolts, 0.5f, 2.5f)) {
    PRINT_NAMED_WARNING("CubeBatteryComponent.HandleCubeVoltageData.InvalidCubeVoltage",
                        "Ignoring invalid cube voltage of %.2f (raw value %u) for cube with factory ID %s",
                        batteryVolts, voltageData.railVoltageCnts, factoryId.c_str());
    return;
  }
  
  // Send a "cube low battery" message if we have crossed the low battery threshold
  const bool isLowBattery = batteryVolts < kCubeLowBatteryThresh_volts;
  if (isLowBattery) {
    float prevVolts = -1.f;
    if (_cubeBatteryVoltages.find(factoryId) != _cubeBatteryVoltages.end()) {
      prevVolts = _cubeBatteryVoltages[factoryId];
    }
    
    if (prevVolts < 0.f || prevVolts >= kCubeLowBatteryThresh_volts) {
      // Either this is the first voltgae reading from this cube and it is low, or we have crossed the low voltage
      // threshold. So send the "low battery" message.
      PRINT_NAMED_WARNING("CubeBatteryComponent.HandleCubeVoltageData.LowCubeBattery",
                          "Low cube battery detected. Voltage %.3f, factoryId %s",
                          batteryVolts, factoryId.c_str());
      
      // Send gateway message
      using namespace external_interface;
      auto* cubeBatteryMsg = new CubeBattery{
        CubeBattery::CubeBatteryLevel::CubeBattery_CubeBatteryLevel_Low,
        factoryId,
        batteryVolts
      };
      _gi->Broadcast(ExternalMessageRouter::Wrap(cubeBatteryMsg));
    }
  }
  
  _cubeBatteryVoltages[factoryId] = batteryVolts;
}
  

}
}
