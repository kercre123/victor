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

#include "util/logging/DAS.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
namespace {
  // According to VIC-5510, the conversion from raw ADC counts to volts goes according to the following:
  // volts = rawCnts * 3.6 / 1024
  constexpr float kBatteryRawCntsToVolts = 3.6f / 1024.f;
  
  // According to hardware team, this is a rough indication of when the cube is "low battery"
  const float kCubeLowBatteryThresh_volts = 1.1f;
  
  // After this many consecutive low battery readings, send a message out indicating "low battery"
  const int kMaxNumConsecutiveLowReadings = 5;
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


std::unique_ptr<external_interface::CubeBattery> CubeBatteryComponent::GetCubeBatteryMsg()
{
  if (_robot == nullptr) {
    return nullptr;
  }
  
  const auto& cubeComms = _robot->GetCubeCommsComponent();
  
  BleFactoryId cubeToSend = "";
  if (cubeComms.IsConnectedToCube()) {
    const auto& connectedCube = cubeComms.GetConnectedCubeFactoryId();
    if (_cubeBatteryInfo.find(connectedCube) != _cubeBatteryInfo.end()) {
      cubeToSend = connectedCube;
    }
  } else {
    // Not currently connected to any cube, so send information of the most recently heard-from cube
    auto mostRecentIt = std::max_element(_cubeBatteryInfo.begin(),
                                         _cubeBatteryInfo.end(),
                                         [](const std::pair<BleFactoryId, CubeBatteryInfo>& p1,
                                            const std::pair<BleFactoryId, CubeBatteryInfo>& p2) {
                                           return p1.second.timeOfLastReading_sec < p2.second.timeOfLastReading_sec;
                                         });
    if (mostRecentIt != _cubeBatteryInfo.end()) {
      cubeToSend = mostRecentIt->first;
    }
  }
  
  if (cubeToSend.empty()) {
    return nullptr;
  }
  
  // Create the message
  const auto& cubeInfo = _cubeBatteryInfo[cubeToSend];
  using namespace external_interface;
  auto msg = std::make_unique<CubeBattery>();
  msg->set_level((cubeInfo.batteryVolts > kCubeLowBatteryThresh_volts) ?
                 CubeBattery_CubeBatteryLevel_Normal :
                 CubeBattery_CubeBatteryLevel_Low);
  msg->set_factory_id(cubeToSend);
  msg->set_battery_volts(cubeInfo.batteryVolts);
  const auto now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  msg->set_time_since_last_reading_sec(now_sec - cubeInfo.timeOfLastReading_sec);
  return msg;
}


float CubeBatteryComponent::GetCubeBatteryVoltage(const BleFactoryId& factoryId)
{
  const auto cubeIt = _cubeBatteryInfo.find(factoryId);
  if (cubeIt == _cubeBatteryInfo.end()) {
    PRINT_NAMED_WARNING("CubeBatteryComponent.GetCubeBatteryVoltage.CubeNotFound",
                        "We have never received a battery voltage message from cube with factory ID %s",
                        factoryId.c_str());
    return -1.f;
  }
  return cubeIt->second.batteryVolts;
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
  
  // If this is the first time we are getting info for this cube, then log its battery voltage now
  const bool hasBattInfo = (_cubeBatteryInfo.find(factoryId) != _cubeBatteryInfo.end());
  if (!hasBattInfo) {
    DASMSG(cube_battery_voltage, "cube.battery_voltage", "Records the cube's battery voltage if this is the first information we are receiving from it");
    DASMSG_SET(i1, (int) std::round(1000.f * batteryVolts), "Cube battery voltage (mV)");
    DASMSG_SET(s1, factoryId, "Cube factory ID");
    DASMSG_SEND();
  }
  
  auto& battInfo = _cubeBatteryInfo[factoryId];
  
  battInfo.batteryVolts = batteryVolts;
  battInfo.timeOfLastReading_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Send a "cube low battery" message if we have crossed the low battery threshold
  const bool isLowBattery = batteryVolts < kCubeLowBatteryThresh_volts;
  if (isLowBattery) {
    ++battInfo.numConsecutiveLowReadings;
    
    // If we have received too many consecutive low battery readings, send the "low battery" message
    if (battInfo.numConsecutiveLowReadings == kMaxNumConsecutiveLowReadings) {
      PRINT_NAMED_WARNING("CubeBatteryComponent.HandleCubeVoltageData.LowCubeBattery",
                          "Low cube battery detected. Voltage %.3f, factoryId %s",
                          batteryVolts, factoryId.c_str());
      DASMSG(cube_low_battery, "cube.low_battery", "Indicates that the connected cube has reported a low battery voltage");
      DASMSG_SET(i1, (int) std::round(1000.f * batteryVolts), "Cube battery voltage (mV)");
      DASMSG_SET(s1, factoryId, "Cube factory ID");
      DASMSG_SEND();
      // Send gateway message
      auto* cubeBatteryMsg = GetCubeBatteryMsg().release();
      DEV_ASSERT(cubeBatteryMsg != nullptr, "CubeBatteryComponent::HandleCubeVoltageData.NullCubeBatteryMsg");
      _gi->Broadcast(ExternalMessageRouter::Wrap(cubeBatteryMsg));
    }
  } else {
    battInfo.numConsecutiveLowReadings = 0;
  }
}
  

}
}
