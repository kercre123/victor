/**
 * File: OSState_vicos.cpp
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
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>
#include <netdb.h>

#include <linux/wireless.h>

#include <fstream>
#include <array>
#include <stdlib.h>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using osState_vicos.cpp
#endif

namespace Anki {
namespace Cozmo {

namespace {

  std::ifstream _cpuFile;
  std::ifstream _tempFile;
  std::ifstream _uptimeFile;
  std::ifstream _memInfoFile;
  std::ifstream _cpuTimeStatsFile;

  const char* kNominalCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
  const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
  const char* kTemperatureFile = "/sys/devices/virtual/thermal/thermal_zone3/temp";
  const char* kMACAddressFile = "/sys/class/net/wlan0/address";
  const char* kRecoveryModeFile = "/data/unbrick";
  const char* kUptimeFile = "/proc/uptime";
  const char* kMemInfoFile = "/proc/meminfo";
  const char* kCPUTimeStatsFile = "/proc/stat";
  const char* kWifiTxBytesFile = "/sys/class/net/wlan0/statistics/tx_bytes";
  const char* kWifiRxBytesFile = "/sys/class/net/wlan0/statistics/rx_bytes";

  // System vars
  uint32_t _cpuFreq_kHz;      // CPU freq
  uint32_t _cpuTemp_C;        // Temperature in Celsius
  float _uptime_s;            // Uptime in seconds
  float _idleTime_s;          // Idle time in seconds
  uint32_t _memTotal_kB;      // Total memory in kB
  uint32_t _memFree_kB;       // Free memory in kB
  std::vector<std::string> _CPUTimeStats; // CPU time stats lines


  // How often state variables are updated
  uint32_t _updatePeriod_ms = 0;
  uint32_t _lastUpdateTime_ms = 0;

} // namespace

// Searches the .prop property files for the given key and returns the value
// __system_get_property() from sys/system_properties.h does
// not work for some reason so we have to read the files manually
std::string GetProperty(const std::string& key)
{
  const std::string kProp = key + "=";

  // First check the regular build.prop
  std::ifstream infile("/build.prop");

  std::string line;
  while(std::getline(infile, line))
  {
    size_t index = line.find(kProp);
    if(index != std::string::npos)
    {
      infile.close();
      return line.substr(kProp.length());
    }
  }

  infile.close();

  // If the key wasn't found in /build.prop, then
  // check the persistent build.prop
  infile.open("/data/persist/build.prop");

  while(std::getline(infile, line))
  {
    size_t index = line.find(kProp);
    if(index != std::string::npos)
    {
      infile.close();
      return line.substr(kProp.length());
    }
  }

  infile.close();

  return "";
}

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

      // Update cpu freq
      UpdateCPUFreq_kHz();

      // Update temperature reading
      UpdateTemperature_C();

      // const auto now = steady_clock::now();
      // const auto elapsed_us = duration_cast<microseconds>(now - startTime).count();
      // PRINT_NAMED_INFO("OSState", "Update took %lld microseconds", elapsed_us);

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
    _osBuildVersion = GetProperty("ro.build.version.release");
  }

  return _osBuildVersion;
}

const std::string& OSState::GetBuildSha()
{
  return _buildSha;
}

const std::string& OSState::GetRobotName() const
{
  static std::string name = GetProperty("anki.robot.name");
  if(name.empty())
  {
    name = GetProperty("anki.robot.name");
  }
  return  name;
}

static std::string GetIPV4AddressForInterface(const char* if_name) {
  struct ifaddrs* ifaddr = nullptr;
  struct ifaddrs* ifa = nullptr;
  char host[NI_MAXHOST] = {0};

  int rc = getifaddrs(&ifaddr);
  if (rc == -1) {
    PRINT_NAMED_ERROR("OSState.GetIPAddress.GetIfAddrsFailed", "%s", strerror(errno));
    return "";
  }

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) {
      continue;
    }

    if (ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    if (!strcmp(ifa->ifa_name, if_name)) {
      break;
    }
  }

  if (ifa != nullptr) {
    int s = getnameinfo(ifa->ifa_addr,
                        sizeof(struct sockaddr_in),
                        host, sizeof(host),
                        NULL, 0, NI_NUMERICHOST);
    if (s != 0) {
      PRINT_NAMED_ERROR("OSState.GetIPAddress.GetNameInfoFailed", "%s", gai_strerror(s));
      memset(host, 0, sizeof(host));
    }
  }

  if (host[0]) {
    PRINT_NAMED_INFO("OSState.GetIPAddress.IPV4AddressFound", "iface = %s , ip = %s",
                     if_name, host);
  } else {
    PRINT_NAMED_INFO("OSState.GetIPAddress.IPV4AddressNotFound", "iface = %s", if_name);
  }
  freeifaddrs(ifaddr);
  return std::string(host);
}

void OSState::UpdateWifiInfo()
{
  const char* const if_name = "wlan0";
  _ipAddress = GetIPV4AddressForInterface(if_name);
  _ssid.clear();

  int fd=socket(AF_INET,SOCK_DGRAM,0);
  if (fd==-1) {
    ASSERT_NAMED_EVENT(false, "OSState.GetIPAddress.OpenSocketFail", "");
    return;
  }
  // Get SSID
  iwreq req;
  strcpy(req.ifr_name, if_name);
  req.u.data.pointer = (iw_statistics*)malloc(sizeof(iw_statistics));
  req.u.data.length = sizeof(iw_statistics);

  const int kSSIDBufferSize = 32;
  char buffer[kSSIDBufferSize];
  memset(buffer, 0, sizeof(buffer));
  req.u.essid.pointer = buffer;
  req.u.essid.length = kSSIDBufferSize;
  if(ioctl(fd, SIOCGIWESSID, &req) == -1)
  {
    close(fd);
    ASSERT_NAMED_EVENT(false, "OSState.UpdateWifiInfo.FailedToGetSSID","%s", strerror(errno));
  }
  close(fd);

  _ssid = std::string(buffer);
}

const std::string& OSState::GetIPAddress(bool update)
{
  if(_ipAddress.empty() || update)
  {
    UpdateWifiInfo();
  }

  return _ipAddress;
}

const std::string& OSState::GetSSID(bool update)
{
  if(_ssid.empty() || update)
  {
    UpdateWifiInfo();
  }

  return _ssid;
}

std::string OSState::GetMACAddress() const
{
  std::ifstream macFile;
  macFile.open(kMACAddressFile);
  if (macFile.is_open()) {
    std::string macStr;
    macFile >> macStr;
    macFile.close();
    return macStr;
  }
  return "";
}

uint64_t OSState::GetWifiTxBytes() const
{
  uint64_t numBytes = 0;
  std::ifstream txFile;
  txFile.open(kWifiTxBytesFile);
  if (txFile.is_open()) {
    txFile >> numBytes;
    txFile.close();
  }
  return numBytes;
}

uint64_t OSState::GetWifiRxBytes() const
{
  uint64_t numBytes = 0;
  std::ifstream rxFile;
  rxFile.open(kWifiRxBytesFile);
  if (rxFile.is_open()) {
    rxFile >> numBytes;
    rxFile.close();
  }
  return numBytes;
}

bool OSState::IsInRecoveryMode()
{
  return Util::FileUtils::FileExists(kRecoveryModeFile);
}

} // namespace Cozmo
} // namespace Anki
