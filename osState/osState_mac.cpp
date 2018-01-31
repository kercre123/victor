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
  _osBuildNum = 12345;
  _ipAddress = "127.0.0.1";
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

uint32_t OSState::GetCPUFreq_kHz() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetCPUFreq_kHz.ZeroUpdate");
  return kNominalCPUFreq_kHz;
}

bool OSState::IsThermalThrottling() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsThermalThrottling.ZeroUpdate");
  return false;
}

uint32_t OSState::GetTemperature_mC() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetTemperature_mC.ZeroUpdate");

  // 65C: randomly chosen temperature at which throttling does not appear to occur
  // on physical robot
  return 65000;  
}
  
const std::string& OSState::GetSerialNumberAsString()
{
  return _serialNumString;
}
  
u32 OSState::GetOSBuildNumber()
{
  return _osBuildNum;
}

std::string OSState::GetIPAddressInternal()
{
  return _ipAddress;
}

// Executes the provided command and returns the output as a string
std::string OSState::ExecCommand(const char* cmd) 
{
  try
  {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
      PRINT_NAMED_WARNING("EngineMessages.ExecCommand.FailedToOpenPipe", "");
      return "";
    }

    while (!feof(pipe.get()))
    {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      {
        result += buffer.data();
      }
    }

    // Remove the last character as it is a newline
    return result.substr(0,result.size()-1);
  }
  catch(...)
  {
    return "";
  }
}

} // namespace Cozmo
} // namespace Anki

