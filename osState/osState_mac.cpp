/**
 * File: OSState_mac.cpp
 *
 * Authors: Kevin Yoon
 * Created: 2017-12-11
 *
 * Description:
 *
 *   Keeps track of OS-level state, mostly for development/debugging purposes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "osState/osState.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include <webots/Supervisor.hpp>

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <array>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using osState_mac.cpp
#endif

namespace Anki {
namespace Cozmo {
  
namespace {
  // Whether or not SetSupervisor() was called
  bool _supervisorIsSet = false;
  webots::Supervisor *_supervisor = nullptr;

  // Pointer to the CozmoBot node in the scene tree
  webots::Node *_robotNode = nullptr;
  
  // Pointer to the CozmoBot batteryVolts field
  webots::Field* _batteryVoltsField = nullptr;
  
  RobotID_t _robotID = DEFAULT_ROBOT_ID;

  uint32_t _updatePeriod_ms = 0;
  
} // namespace

OSState::OSState()
{
  DEV_ASSERT(_supervisorIsSet, "OSState.Ctor.SupervisorNotSet");

  if (_supervisor != nullptr) {
    // Set RobotID
    const auto* robotIDField = _supervisor->getSelf()->getField("robotID");
    DEV_ASSERT(robotIDField != nullptr, "OSState.Ctor.MissingRobotIDField");
    _robotID = robotIDField->getSFInt32();
    
    // Grab a pointer to the CozmoBot node
    const auto* rootChildren = _supervisor->getRoot()->getField("children");
    for (int i=0 ; i < rootChildren->getCount() ; i++) {
      webots::Node* nd = rootChildren->getMFNode(i);
      if (nd->getTypeName() == "CozmoBot2") {
        _robotNode = nd;
        break;
      }
    }
    
    DEV_ASSERT(_robotNode != nullptr, "OSState.Ctor.NullRobotNode");
    _batteryVoltsField = _robotNode->getField("batteryVolts");
    DEV_ASSERT(_batteryVoltsField != nullptr, "OSState.Ctor.NullBatteryVoltsField");
  }
  
  // Set simulated attributes
  _serialNumString = "12345";
  _osBuildVersion = "12345";
  _ipAddress = "127.0.0.1";
  _buildSha = ANKI_BUILD_SHA;
}

OSState::~OSState()
{
}

void OSState::SetSupervisor(webots::Supervisor *sup)
{
  _supervisor = sup;
  _supervisorIsSet = true;
}

void OSState::Update()
{
}

RobotID_t OSState::GetRobotID() const
{
  return _robotID;
}

void OSState::SetUpdatePeriod(uint32_t milliseconds)
{
  _updatePeriod_ms = milliseconds;
}

bool OSState::IsCPUThrottling() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsCPUThrottling.ZeroUpdate");
  return false;
}

uint32_t OSState::GetCPUFreq_kHz() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetCPUFreq_kHz.ZeroUpdate");
  return kNominalCPUFreq_kHz;
}

uint32_t OSState::GetTemperature_C() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetTemperature_C.ZeroUpdate");

  // 65C: randomly chosen temperature at which throttling does not appear to occur
  // on physical robot
  return 65;  
}


uint32_t OSState::GetBatteryVoltage_uV() const
{
  return Util::numeric_cast<uint32_t>(_batteryVoltsField->getSFFloat() * 1'000'000.f);
}


const std::string& OSState::GetSerialNumberAsString()
{
  return _serialNumString;
}
  
const std::string& OSState::GetOSBuildVersion()
{
  return _osBuildVersion;
}
  
const std::string& OSState::GetBuildSha()
{
  return _buildSha;
}

std::string OSState::GetIPAddressInternal()
{
  return _ipAddress;
}

std::string OSState::GetRobotName() const
{
  return "Vector_0000";
}

} // namespace Cozmo
} // namespace Anki

