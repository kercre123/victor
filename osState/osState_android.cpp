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
  std::ifstream _batteryVoltageFile;
  std::ifstream _uptimeFile;
  std::ifstream _memInfoFile;
  std::ifstream _cpuTimeStatsFile;

  const char* kNominalCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
  const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
  const char* kTemperatureFile = "/sys/devices/virtual/thermal/thermal_zone7/temp";
  const char* kBatteryVoltageFile = "/sys/devices/soc/qpnp-linear-charger-8/power_supply/battery/voltage_now";
  const char* kUptimeFile = "/proc/uptime";
  const char* kMemInfoFile = "/proc/meminfo";
  const char* kCPUTimeStatsFile = "/proc/stat";

  // System vars
  uint32_t _cpuFreq_kHz;      // CPU freq
  uint32_t _cpuTemp_C;        // Temperature in Celsius
  float _batteryVoltage_uV;   // Battery charge in milli-volts
  float _uptime_s;            // Uptime in seconds
  float _idleTime_s;          // Idle time in seconds
  uint32_t _memTotal_kB;      // Total memory in kB
  uint32_t _memFree_kB;       // Free memory in kB
  std::vector<std::string> _CPUTimeStats; // CPU time stats lines


  // How often state variables are updated
  uint32_t _updatePeriod_ms = 0;
  uint32_t _lastUpdateTime_ms = 0;

} // namespace

OSState::OSState()
{
  // Get nominal CPU frequency for this robot
  _tempFile.open(kNominalCPUFreqFile, std::ifstream::in);
  if(_tempFile.is_open()) {
    _tempFile >> kNominalCPUFreq_kHz;
    PRINT_NAMED_INFO("OSState.Constructor.NominalCPUFreq", "%dkHz", kNominalCPUFreq_kHz);
    _tempFile.close();
  }
  else {
    PRINT_NAMED_WARNING("OSState.Constructor.FailedToOpenNominalCPUFreqFile", "%s", kNominalCPUFreqFile);
  }
  
  _cpuFreq_kHz = kNominalCPUFreq_kHz;
  _cpuTemp_C = 0;

  _buildSha = ANKI_BUILD_SHA;
}

OSState::~OSState()
{
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
      
      using namespace std::chrono;
      const auto startTime = steady_clock::now();

      // Update cpu freq
      UpdateCPUFreq_kHz();

      // Update temperature reading
      UpdateTemperature_C();
      
      // Update battery voltage
      UpdateBatteryVoltage_uV();

      const auto now = steady_clock::now();
      const auto elapsed_us = duration_cast<microseconds>(now - startTime).count();
      PRINT_NAMED_INFO("OSState", "Update took %lld microseconds", elapsed_us);

      _lastUpdateTime_ms = now_ms;
    }
  }
}

void OSState::SetUpdatePeriod(uint32_t milliseconds)
{
  _updatePeriod_ms = milliseconds;
}

void OSState::UpdateCPUFreq_kHz() const
{
  // Update cpu freq
  _cpuFile.open(kCPUFreqFile, std::ifstream::in);
  _cpuFile >> _cpuFreq_kHz;
  _cpuFile.close();
}

void OSState::UpdateTemperature_C() const
{
  // Update temperature reading
  _tempFile.open(kTemperatureFile, std::ifstream::in);
  _tempFile >> _cpuTemp_C;
  _tempFile.close();
}

void OSState::UpdateBatteryVoltage_uV() const
{
  // Update battery voltage reading
  _batteryVoltageFile.open(kBatteryVoltageFile, std::ifstream::in);
  _batteryVoltageFile >> _batteryVoltage_uV;
  _batteryVoltageFile.close();
}

void OSState::UpdateUptimeAndIdleTime() const
{
  // Update uptime and idle time data
  _uptimeFile.open(kUptimeFile, std::ifstream::in);
  _uptimeFile >> _uptime_s >> _idleTime_s;
  _uptimeFile.close();
}

void OSState::UpdateMemoryInfo() const
{
  // Update total and free memory
  _memInfoFile.open(kMemInfoFile, std::ifstream::in);
  std::string discard;
  _memInfoFile >> discard >> _memTotal_kB >> discard >> discard >> _memFree_kB;
  _memInfoFile.close();
}

void OSState::UpdateCPUTimeStats() const
{
  // Update CPU time stats lines
  _cpuTimeStatsFile.open(kCPUTimeStatsFile, std::ifstream::in);
  static const int kNumCPUTimeStatLines = 5;
  _CPUTimeStats.resize(kNumCPUTimeStatLines);
  for (int i = 0; i < kNumCPUTimeStatLines; i++) {
    std::getline(_cpuTimeStatsFile, _CPUTimeStats[i]);
  }
  _cpuTimeStatsFile.close();
}

uint32_t OSState::GetCPUFreq_kHz() const
{
  if (_updatePeriod_ms == 0) {
    UpdateCPUFreq_kHz();
  }
  return _cpuFreq_kHz;
}


bool OSState::IsCPUThrottling() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsCPUThrottling.ZeroUpdate");
  return (_cpuFreq_kHz < kNominalCPUFreq_kHz);
}

uint32_t OSState::GetTemperature_C() const
{
  if (_updatePeriod_ms == 0) {
    UpdateTemperature_C();
  }
  return _cpuTemp_C;
}

uint32_t OSState::GetBatteryVoltage_uV() const
{
  if (_updatePeriod_ms == 0) {
    UpdateBatteryVoltage_uV();
  }
  return _batteryVoltage_uV;
}

float OSState::GetUptimeAndIdleTime(float &idleTime_s) const
{
  // Better to have this relatively expensive call as on-demand only
  DEV_ASSERT(_updatePeriod_ms == 0, "OSState.GetUptimeAndIdleTime.NonZeroUpdate");
  if (_updatePeriod_ms == 0) {
    UpdateUptimeAndIdleTime();
  }
  idleTime_s = _idleTime_s;
  return _uptime_s;
}

uint32_t OSState::GetMemoryInfo(uint32_t &freeMem_kB) const
{
  // Better to have this relatively expensive call as on-demand only
  DEV_ASSERT(_updatePeriod_ms == 0, "OSState.GetMemoryInfo.NonZeroUpdate");
  if (_updatePeriod_ms == 0) {
    UpdateMemoryInfo();
  }
  freeMem_kB = _memFree_kB;
  return _memTotal_kB;
}

const std::vector<std::string>& OSState::GetCPUTimeStats() const
{
  // Better to have this relatively expensive call as on-demand only
  DEV_ASSERT(_updatePeriod_ms == 0, "OSState.GetCPUTimeStats.NonZeroUpdate");
  if (_updatePeriod_ms == 0) {
    UpdateCPUTimeStats();
  }
  return _CPUTimeStats;
}


const std::string& OSState::GetSerialNumberAsString()
{
  if(_serialNumString.empty())
  {
    std::ifstream infile("/proc/cmdline");

    std::string line;
    while(std::getline(infile, line))
    {
      static const std::string kProp = "androidboot.serialno=";
      size_t index = line.find(kProp);
      if(index != std::string::npos)
      {
        _serialNumString = line.substr(index + kProp.length(), 8);
      }
    }

    infile.close();
  }
  
  return _serialNumString;
}

const std::string& OSState::GetOSBuildVersion()
{
  if(_osBuildVersion.empty())
  {
    std::ifstream infile("/build.prop");

    std::string line;
    while(std::getline(infile, line))
    {
      static const std::string kProp = "ro.build.version.release=";
      size_t index = line.find(kProp);
      if(index != std::string::npos)
      {
        _osBuildVersion = line.substr(kProp.length(), 12);
      }
    }

    infile.close();
  }
  
  return _osBuildVersion;
}

const std::string& OSState::GetBuildSha() 
{
  return _buildSha;
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
    ASSERT_NAMED_EVENT(false, "OSState.GetIPAddress.InvalidInterfaceName", "");
  }

  int fd=socket(AF_INET,SOCK_DGRAM,0);
  if (fd==-1) {
    ASSERT_NAMED_EVENT(false, "OSState.GetIPAddress.OpenSocketFail", "");
  }

  if (ioctl(fd,SIOCGIFADDR,&ifr)==-1) {
    int temp_errno=errno;
    close(fd);
    ASSERT_NAMED_EVENT(false, "OSState.GetIPAddress.IoctlError", "%s", strerror(temp_errno));
  }
  close(fd);

  struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
  return std::string(inet_ntoa(ipaddr->sin_addr));
}

} // namespace Cozmo
} // namespace Anki

