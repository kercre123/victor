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
  }
  
  // Set simulated attributes
  _serialNumString = "12345";
  _osBuildVersion = "12345";
  _ipAddress = "127.0.0.1";
  _ssid = "AnkiNetwork";
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
  
const std::string& OSState::GetSerialNumberAsString()
{
  return _serialNumString;
}
  
const std::string& OSState::GetOSBuildVersion()
{
  return _osBuildVersion;
}

std::string OSState::GetMACAddress() const
{
  return "00:00:00:00:00:00";
}

const std::string& OSState::GetIPAddress(bool update)
{
  return _ipAddress;
}

const std::string& OSState::GetSSID(bool update)
{
  return _ssid;
}

std::string OSState::GetRobotName() const
{
  return "Vector_0000";
}

bool OSState::IsInRecoveryMode()
{
  return false;
}

} // namespace Cozmo
} // namespace Anki

