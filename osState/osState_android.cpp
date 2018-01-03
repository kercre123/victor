/**
 * File: OSState_android.cpp
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
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <fstream>
#include <array>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using osState_android.cpp
#endif

namespace Anki {
namespace Cozmo {
  
namespace {

  std::ifstream _cpuFile;
  std::ifstream _tempFile;

  const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
  const char* kTemperatureFile = "/sys/devices/virtual/thermal/thermal_zone8/temp";

  // System vars
  uint32_t _cpuFreq_kHz; // CPU freq
  uint32_t _cpuTemp_mC;  // Temperature in milli-Celsius

  // How often state variables are updated
  uint32_t _updatePeriod_ms = 0;
  uint32_t _lastUpdateTime_ms = 0;

} // namespace
  
OSState::OSState()
{
  _cpuFreq_kHz = kNominalCPUFreq_kHz;
  _cpuTemp_mC = 0;

  _tempFile.open(kTemperatureFile, std::ifstream::in);
  _cpuFile.open(kCPUFreqFile, std::ifstream::in);
}

OSState::~OSState()
{
  if (_tempFile.is_open()) {
    _tempFile.close();
  }
  if (_cpuFile.is_open()) {
    _cpuFile.close();
  }
}

RobotID_t OSState::GetRobotID() const
{
  return DEFAULT_ROBOT_ID;
}

void OSState::Update()
{
  if (_updatePeriod_ms != 0) {
    const double now_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    if (now_ms - _lastUpdateTime_ms > _updatePeriod_ms) {
      
      // Update cpu freq
      _cpuFile.seekg(0, _cpuFile.beg);
      _cpuFile >> _cpuFreq_kHz;
      
      // Update temperature reading
      _tempFile.seekg(0, _tempFile.beg);
      _tempFile >> _cpuTemp_mC;

      _lastUpdateTime_ms = now_ms;
    }
  }
}

void OSState::SetUpdatePeriod(uint32_t milliseconds)
{
  _updatePeriod_ms = milliseconds;
}

uint32_t OSState::GetCPUFreq_kHz() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetCPUFreq_kHz.ZeroUpdate");
  return _cpuFreq_kHz;
}

bool OSState::IsThermalThrottling() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsThermalThrottling.ZeroUpdate");
  return (_cpuFreq_kHz < kNominalCPUFreq_kHz);
}

uint32_t OSState::GetTemperature_mC() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetTemperature_mC.ZeroUpdate");
  return _cpuTemp_mC;
}

std::string OSState::GetIPAddressInternal()
{
  // Open a socket to figure out the ip adress of the wlan0 (wifi) interface
  const char* const if_name = "wlan0";
  struct ifreq ifr;
  size_t if_name_len=strlen(if_name);
  if (if_name_len<sizeof(ifr.ifr_name)) {
    memcpy(ifr.ifr_name,if_name,if_name_len);
    ifr.ifr_name[if_name_len]=0;
  } else {
    ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.InvalidInterfaceName", "");
  }

  int fd=socket(AF_INET,SOCK_DGRAM,0);
  if (fd==-1) {
    ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.OpenSocketFail", "");
  }

  if (ioctl(fd,SIOCGIFADDR,&ifr)==-1) {
    int temp_errno=errno;
    close(fd);
    ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.IoctlError", "%s", strerror(temp_errno));
  }
  close(fd);

  struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
  return std::string(inet_ntoa(ipaddr->sin_addr));
}

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

