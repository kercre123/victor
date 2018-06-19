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
#include "util/console/consoleInterface.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include "util/time/universalTime.h"

#include "cutils/properties.h"

#include "anki/cozmo/shared/factory/emrHelper.h"

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
#include <sys/timex.h>
#include <sys/stat.h>

#include <linux/wireless.h>

#include <fstream>
#include <array>
#include <iomanip>
#include <stdlib.h>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using osState_vicos.cpp
#endif

namespace Anki {
namespace Cozmo {

CONSOLE_VAR_ENUM(int, kWebvizUpdatePeriod, "OSState", 0, "Off,10ms,100ms,1000ms,10000ms");
CONSOLE_VAR(bool, kOSState_FakeNoTime, "OSState", false);
CONSOLE_VAR(bool, kOSState_FakeNoTimezone, "OSState", false);

namespace {
  uint32_t kPeriodEnumToMS[] = {0, 10, 100, 1000, 10000};

  std::ifstream _cpuFile;
  std::ifstream _temperatureFile;
  std::ifstream _uptimeFile;
  std::ifstream _memInfoFile;
  std::ifstream _cpuTimeStatsFile;

  std::mutex _cpuMutex;
  std::mutex _temperatureMutex;
  std::mutex _MACAddressMutex;
  std::mutex _uptimeMutex;
  std::mutex _memInfoMutex;
  std::mutex _cpuTimeStatsMutex;

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
  const char* kBootIDFile = "/proc/sys/kernel/random/boot_id";
  const char* kLocalTimeFile = "/data/etc/localtime";
  constexpr const char* kUniversalTimeFile = "/usr/share/zoneinfo/Universal";
  constexpr const char* kRobotVersionFile = "/anki/etc/version";

  // System vars
  uint32_t _cpuFreq_kHz;      // CPU freq
  uint32_t _cpuTemp_C;        // Temperature in Celsius
  float _uptime_s;            // Uptime in seconds
  float _idleTime_s;          // Idle time in seconds
  uint32_t _memTotal_kB;      // Total memory in kB
  uint32_t _memFree_kB;       // Free memory in kB
  uint32_t _memAvailable_kb;  // Available memory in kB
  std::vector<std::string> _CPUTimeStats; // CPU time stats lines


  // How often state variables are updated
  uint32_t _updatePeriod_ms = 0;
  uint32_t _lastUpdateTime_ms = 0;
  uint32_t _lastWebvizUpdateTime_ms = 0;

  std::function<void(const Json::Value&)> _webServiceCallback = nullptr;

  // helper to allow compile-time static_asserts on length of a const string
  constexpr int GetConstStrLength(const char* str)
  {
    return *str ? 1 + GetConstStrLength(str + 1) : 0;
  }


} // namespace

std::string GetProperty(const std::string& key)
{
  char propBuf[PROPERTY_VALUE_MAX] = {0};
  int rc = property_get(key.c_str(), propBuf, "");
  if(rc <= 0)
  {
    PRINT_NAMED_WARNING("OSState.GetProperty.FailedToFindProperty",
                        "Property %s not found",
                        key.c_str());
  }

  return std::string(propBuf);
}

OSState::OSState()
{
  // Suppress unused function error for WriteEMR
  (void)static_cast<void(*)(size_t, void*, size_t)>(Factory::WriteEMR);
  (void)static_cast<void(*)(size_t, uint32_t)>(Factory::WriteEMR);

  // Get nominal CPU frequency for this robot
  std::ifstream file;
  file.open(kNominalCPUFreqFile, std::ifstream::in);
  if(file.is_open()) {
    file >> kNominalCPUFreq_kHz;
    PRINT_NAMED_INFO("OSState.Constructor.NominalCPUFreq", "%dkHz", kNominalCPUFreq_kHz);
    file.close();
  }
  else {
    PRINT_NAMED_WARNING("OSState.Constructor.FailedToOpenNominalCPUFreqFile", "%s", kNominalCPUFreqFile);
  }

  _cpuFreq_kHz = kNominalCPUFreq_kHz;
  _cpuTemp_C = 0;

  _buildSha = ANKI_BUILD_SHA;

  _lastWebvizUpdateTime_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
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
      // const auto startTime = steady_clock::now();

      // Update cpu freq
      UpdateCPUFreq_kHz();

      // Update temperature reading
      UpdateTemperature_C();

      // Update memory info
      UpdateMemoryInfo();

      // const auto now = steady_clock::now();
      // const auto elapsed_us = duration_cast<microseconds>(now - startTime).count();
      // PRINT_NAMED_INFO("OSState", "Update took %lld microseconds", elapsed_us);

      _lastUpdateTime_ms = now_ms;

    }
  }

  if (kWebvizUpdatePeriod != 0 && _webServiceCallback) {
    const double now_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    if (now_ms - _lastWebvizUpdateTime_ms > kPeriodEnumToMS[kWebvizUpdatePeriod]) {
      UpdateCPUTimeStats();

      Json::Value json;
      json["deltaTime_ms"] = now_ms - _lastWebvizUpdateTime_ms;
      auto& usage = json["usage"];
      for(size_t i = 0; i < _CPUTimeStats.size(); ++i) {
        usage.append( _CPUTimeStats[i] );
      }
      _webServiceCallback(json);

      _lastWebvizUpdateTime_ms = now_ms;
    }
  }
}

void OSState::SetUpdatePeriod(uint32_t milliseconds)
{
  _updatePeriod_ms = milliseconds;
}

void OSState::SendToWebVizCallback(const std::function<void(const Json::Value&)>& callback) {
  _webServiceCallback = callback;
}

void OSState::UpdateCPUFreq_kHz() const
{
  // Update cpu freq
  std::lock_guard<std::mutex> lock(_cpuMutex);
  _cpuFile.open(kCPUFreqFile, std::ifstream::in);
  if (_cpuFile.is_open()) {
    _cpuFile >> _cpuFreq_kHz;
    _cpuFile.close();
  }
}

void OSState::UpdateTemperature_C() const
{
  // Update temperature reading
  std::lock_guard<std::mutex> lock(_temperatureMutex);
  _temperatureFile.open(kTemperatureFile, std::ifstream::in);
  if (_temperatureFile.is_open()) {
    _temperatureFile >> _cpuTemp_C;
    _temperatureFile.close();
  }
}

void OSState::UpdateUptimeAndIdleTime() const
{
  // Update uptime and idle time data
  std::lock_guard<std::mutex> lock(_uptimeMutex);
  _uptimeFile.open(kUptimeFile, std::ifstream::in);
  if (_uptimeFile.is_open()) {
    _uptimeFile >> _uptime_s >> _idleTime_s;
    _uptimeFile.close();
  }
}

void OSState::UpdateMemoryInfo() const
{
  // Update total and free memory
  std::lock_guard<std::mutex> lock(_memInfoMutex);
  _memInfoFile.open(kMemInfoFile, std::ifstream::in);
  if (_memInfoFile.is_open()) {
    std::string discard;
    _memInfoFile >> discard >> _memTotal_kB >> discard >> discard >> _memFree_kB >> discard >> discard >> _memAvailable_kb;
    _memInfoFile.close();
  }
}

void OSState::UpdateCPUTimeStats() const
{
  // Update CPU time stats lines
  std::lock_guard<std::mutex> lock(_cpuTimeStatsMutex);
  _cpuTimeStatsFile.open(kCPUTimeStatsFile, std::ifstream::in);
  if (_cpuTimeStatsFile.is_open()) {
    static const int kNumCPUTimeStatLines = 5;
    _CPUTimeStats.resize(kNumCPUTimeStatLines);
    for (int i = 0; i < kNumCPUTimeStatLines; i++) {
      std::getline(_cpuTimeStatsFile, _CPUTimeStats[i]);
    }
    _cpuTimeStatsFile.close();
  }
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

uint32_t OSState::GetMemoryInfo(uint32_t &freeMem_kB, uint32_t &availableMem_kB) const
{
  // Better to have this relatively expensive call as on-demand only
  DEV_ASSERT(_updatePeriod_ms == 0, "OSState.GetMemoryInfo.NonZeroUpdate");
  if (_updatePeriod_ms == 0) {
    UpdateMemoryInfo();
  }
  freeMem_kB = _memFree_kB;
  availableMem_kB = _memAvailable_kb;
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
    std::stringstream ss;
    ss << std::hex
       << std::setfill('0')
       << std::setw(8)
       << std::uppercase
       << Factory::GetEMR()->fields.ESN;
    _serialNumString = ss.str();
  }
  return _serialNumString;
}

const std::string& OSState::GetOSBuildVersion()
{
  if(_osBuildVersion.empty())
  {
    _osBuildVersion = GetProperty("ro.build.display.id");
  }

  return _osBuildVersion;
}

const std::string& OSState::GetRobotVersion()
{
  if (_robotVersion.empty()) {
    std::ifstream ifs(kRobotVersionFile);
    ifs >> _robotVersion;
    Anki::Util::StringTrimWhitespaceFromEnd(_robotVersion);
  }
  return _robotVersion;
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

static std::string GetWiFiSSIDForInterface(const char* if_name) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    ASSERT_NAMED_EVENT(false, "OSState.GetSSID.OpenSocketFail", "");
    return "";
  }

  iwreq req;
  memset(&req, 0, sizeof(req));
  (void) strncpy(req.ifr_name, if_name, sizeof(req.ifr_name) - 1);
  char essid[IW_ESSID_MAX_SIZE + 2] = {0};
  req.u.essid.pointer = essid;
  req.u.essid.length = sizeof(essid) - 2;

  if (ioctl(fd, SIOCGIWESSID, &req) == -1) {
    PRINT_NAMED_INFO("OSState.UpdateWifiInfo.FailedToGetSSID", "iface = %s , errno = %s",
                     if_name, strerror(errno));
    memset(essid, 0, sizeof(essid));
  }
  (void) close(fd);
  PRINT_NAMED_INFO("OSState.GetSSID", "%s", essid);
  return std::string(essid);
}

void OSState::UpdateWifiInfo()
{
  const char* const if_name = "wlan0";
  _ipAddress = GetIPV4AddressForInterface(if_name);
  _ssid = GetWiFiSSIDForInterface(if_name);
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
  std::lock_guard<std::mutex> lock(_MACAddressMutex);
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

bool OSState::HasValidEMR() const
{
  const auto * emr = Factory::GetEMR();
  return (emr != nullptr);
}

const std::string & OSState::GetBootID()
{
  if (_bootID.empty()) {
    // http://0pointer.de/blog/projects/ids.html
    _bootID = Util::FileUtils::ReadFile(kBootIDFile);
    Anki::Util::StringTrimWhitespaceFromEnd(_bootID);
    if (_bootID.empty()) {
      LOG_ERROR("OSState.GetBootID", "Unable to read boot ID from %s", kBootIDFile);
    }
  }
  return _bootID;
}

bool OSState::IsWallTimeSynced() const
{
  if (kOSState_FakeNoTime) {
    return false;
  }

  struct timex txc = {};

  if (adjtimex(&txc) < 0) {
    LOG_ERROR("OSState.IsWallTimeSynced.CantGetTimex", "Invalid return from adjtimex");
    return false;
  }

  if (txc.status & STA_UNSYNC) {
    return false;
  }

  return true;
}

bool OSState::HasTimezone() const
{
  if (kOSState_FakeNoTimezone) {
    return false;
  }

  if (!Util::FileUtils::FileExists(kUniversalTimeFile)) {
    LOG_ERROR("OSState.HasTimezone.NoUniversalTimeFile",
              "Unable to find universal time file '%s', cant check for timezone (assuming none)",
              kUniversalTimeFile);
    return false;
  }

  if (!Util::FileUtils::FileExists(kLocalTimeFile)) {
    LOG_ERROR("OSState.HasTimezone.NoLocalTimeFile",
              "Missing local time file '%s'",
              kLocalTimeFile);
    return false;
  }

  // local time should be a symlink to something, either Universal (meaning we don't have a timezone) or a
  // specific timezone
  struct stat linkStatus;
  const int ok = lstat(kLocalTimeFile, &linkStatus);
  if (ok < 0) {
    LOG_ERROR("OSState.HasTimezone.CantStatLink",
              "lstat(%s) returned %d, error %s",
              kLocalTimeFile,
              ok,
              strerror(errno));
    return false;
  }

  if (!S_ISLNK(linkStatus.st_mode)) {
    LOG_ERROR("OSState.HasTimezone.LocalTimeNotALink",
              "Local time file '%s' exists but isn't a symlink",
              kLocalTimeFile);
    return false;
  }

  // check which file the kLocalTimeFile is a symlink to

  // the path string length can be variable. Rather than dynamically allocating it, just make sure our
  // statically allocated buffer is large enough
  static const size_t linkPathLen = 1024;
  static_assert (GetConstStrLength(kUniversalTimeFile) < linkPathLen,
                 "OSState.HasTimezone.InvalidFilePath");

  if( linkStatus.st_size >= linkPathLen ) {
    LOG_ERROR("OSState.HasTimezone.LinkNameTooLong",
              "Link path size is %ld, but we only made room for %zu",
              linkStatus.st_size,
              linkPathLen);
    // this means it can't be pointing to kUniversalTimeFile (we static assert that that path will fit within
    // the buffer), so it must be some really long file. It seems likely that this is a timezone with a long
    // name, so return true, but it technically could be pointing to any file
    return true;
  }

  char linkPath[linkPathLen];
  const ssize_t written = readlink(kLocalTimeFile, linkPath, linkPathLen);
  if (written < 0 || written >= linkPathLen) {
    LOG_ERROR("OSState.HasTimezone.CantReadLink",
              "File '%s' looks like a symlink, but can't be read (returned %d, error %s)",
              kLocalTimeFile,
              written,
              strerror(errno));
    return false;
  }

  linkPath[written] = '\0';

  // if timezone isn't set, path is either kUniversalTimeFile or ../../kUniversalTimeFile
  const char* found = strstr(linkPath, kUniversalTimeFile);
  if (nullptr == found) {
    // string doesn't match, so the link is pointing to some other file

    if( Util::FileUtils::FileExists(linkPath) ) {
      // valid file to link to (assume it's a time zone)
      return true;
    }
    else {
      LOG_ERROR("OSState.HasTimezone.InvalidSymLink",
                "File '%s' is a sym link to '%s' which does not exist",
                kLocalTimeFile,
                linkPath);
      return false;
    }
  }

  if (found != linkPath) {
    // double check that it's just prefixed with ../../
    if (found != linkPath + 5 ||
        strstr(linkPath, "../../") != linkPath) {
      LOG_WARNING("OSState.HasTimezone.InvalidPath",
                  "'%s' is a symlink to '%s' which doesn't meet expectations",
                  kLocalTimeFile,
                  linkPath);
    }
  }

  // since kUniversalTimeFile is being linked to, we don't have a timezone
  return false;
}

} // namespace Cozmo
} // namespace Anki
