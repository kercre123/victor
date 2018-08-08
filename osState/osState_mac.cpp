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
#include "util/console/consoleInterface.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"

#include <webots/Supervisor.hpp>

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <array>

#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using osState_mac.cpp
#endif

namespace Anki {
namespace Vector {

CONSOLE_VAR_ENUM(int, kWebvizUpdatePeriod, "OSState", 0, "Off,10ms,100ms,1000ms,10000ms");

namespace {
  uint32_t kPeriodEnumToMS[] = {0, 10, 100, 1000, 10000};

  // Whether or not SetSupervisor() was called
  bool _supervisorIsSet = false;
  webots::Supervisor *_supervisor = nullptr;

  RobotID_t _robotID = DEFAULT_ROBOT_ID;

  // System vars
  uint32_t _cpuFreq_kHz;      // CPU freq
  uint32_t _cpuTemp_C;        // Temperature in Celsius
  float _uptime_s;            // Uptime in seconds
  float _idleTime_s;          // Idle time in seconds
  uint32_t _memTotal_kB;      // Total memory in kB
  uint32_t _memFree_kB;       // Free memory in kB
  uint32_t _memAvailable_kB;  // Available memory in kB
  std::vector<std::string> _CPUTimeStats; // CPU time stats lines

  // How often state variables are updated
  uint32_t _updatePeriod_ms = 0;
  uint32_t _lastUpdateTime_ms = 0;
  uint32_t _lastWebvizUpdateTime_ms = 0;

  std::function<void(const Json::Value&)> _webServiceCallback = nullptr;

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
  _robotVersion = "0.0.0";
  _ipAddress = "127.0.0.1";
  _ssid = "AnkiNetwork";

  _cpuFreq_kHz = kNominalCPUFreq_kHz;
  _cpuTemp_C = 0;

  _buildSha = ANKI_BUILD_SHA;

  _lastWebvizUpdateTime_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
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
  if (_updatePeriod_ms != 0) {
    const double now_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    if (now_ms - _lastUpdateTime_ms > _updatePeriod_ms) {

      // Update cpu freq
      UpdateCPUFreq_kHz();

      // Update temperature reading
      UpdateTemperature_C();

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

RobotID_t OSState::GetRobotID() const
{
  return _robotID;
}

void OSState::UpdateCPUFreq_kHz() const
{
  // Update cpu freq
  uint32_t frequency;
  size_t sizeOfFreq = sizeof(frequency);
  int mib[2] = {CTL_HW, HW_CPU_FREQ};

  if(sysctl(mib, 2, &frequency, &sizeOfFreq, NULL, 0) == 0) {
    _cpuFreq_kHz = frequency/(1024*1024);
  } else {
    _cpuFreq_kHz = kNominalCPUFreq_kHz;
  }
}

void OSState::SetDesiredCPUFrequency(DesiredCPUFrequency freq)
{
  // not supported on mac
}

void OSState::UpdateTemperature_C() const
{
  // Update temperature reading

  // 65C: randomly chosen temperature at which throttling does not appear to occur
  // on physical robot
  _cpuTemp_C = 65;
}

void OSState::UpdateUptimeAndIdleTime() const
{
  // Update uptime time data, idle time data is not calculated
  _uptime_s = 0;
  _idleTime_s = 0;

  struct timeval boottime;
  size_t sizeOfBoottime = sizeof(boottime);
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};

  if (sysctl(mib, 2, &boottime, &sizeOfBoottime, NULL, 0) == 0)
  {
    time_t bsec = boottime.tv_sec;
    time_t csec = time(NULL);

    _uptime_s = difftime(csec, bsec);
  }
}

void OSState::UpdateMemoryInfo() const
{
  // Update total and free memory
  _memTotal_kB = 0;
  _memFree_kB = 0;

  struct task_basic_info info;
  mach_msg_type_number_t sizeOfInfo = sizeof(info);

  kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &sizeOfInfo);
  if (kerr == KERN_SUCCESS) {
    _memTotal_kB = static_cast<uint32_t>(info.resident_size / 1024);
  }

  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
  vm_statistics_data_t vmstat;

  kerr = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count);
  if (kerr == KERN_SUCCESS)
  {
    _memFree_kB = static_cast<uint32_t>(vmstat.free_count / 1024);
  }
  
  // TODO: differentiate available and free
  _memAvailable_kB = _memFree_kB;
}

void OSState::UpdateCPUTimeStats() const
{
  // Update CPU time stats lines
  unsigned int numCPUs;
  size_t sizeOfNumCPUs = sizeof(numCPUs);
  int mib[2] = {CTL_HW, HW_NCPU};

  if (sysctl(mib, 2, &numCPUs, &sizeOfNumCPUs, NULL, 0) != 0) {
    numCPUs = 1;
  }

  natural_t numCPUsU = 0;
  processor_info_array_t cpuInfo;
  mach_msg_type_number_t numCpuInfo;

  kern_return_t kerr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &numCPUsU, &cpuInfo, &numCpuInfo);
  if (kerr == KERN_SUCCESS) {
    integer_t total[CPU_STATE_MAX] = {0};

    _CPUTimeStats.resize(numCPUs+1);
    for (natural_t i = 0; i < numCPUs; ++i) {
      char temp[79+1];
      snprintf(temp, sizeof(temp), "CPU%d %d %d %d %d 0 0 0 0 0 0", i, cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_USER], cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_NICE], cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_SYSTEM], cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_IDLE]);
      _CPUTimeStats[i+1] = temp;

      total[CPU_STATE_USER] += cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_USER];
      total[CPU_STATE_NICE] += cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_NICE];
      total[CPU_STATE_SYSTEM] += cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_SYSTEM];
      total[CPU_STATE_IDLE] += cpuInfo[(CPU_STATE_MAX*i)+CPU_STATE_IDLE];
    }

    char temp[79+1];
    snprintf(temp, sizeof(temp), "CPU %d %d %d %d 0 0 0 0 0 0", total[CPU_STATE_USER], total[CPU_STATE_NICE], total[CPU_STATE_SYSTEM], total[CPU_STATE_IDLE]);
    _CPUTimeStats[0] = temp;
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
  return false;
}

uint32_t OSState::GetTemperature_C() const
{
  if (_updatePeriod_ms == 0) {
    UpdateTemperature_C();
  }
  return _cpuTemp_C;
}

const std::string& OSState::GetSerialNumberAsString()
{
  return _serialNumString;
}

const std::string& OSState::GetOSBuildVersion()
{
  return _osBuildVersion;
}
  
void OSState::GetOSBuildVersion(int& major, int& minor, int& incremental) const
{
  // always the latest for the purposes of testing
  major = std::numeric_limits<int>::max();
  minor = std::numeric_limits<int>::max();
  incremental = std::numeric_limits<int>::max();
}

const std::string& OSState::GetRobotVersion()
{
  return _robotVersion;
}

const std::string& OSState::GetBuildSha()
{
  return _buildSha;
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

uint64_t OSState::GetWifiTxBytes() const
{
  return 0;
}

uint64_t OSState::GetWifiRxBytes() const
{
  return 0;
}

float OSState::GetUptimeAndIdleTime(float &idleTime_s) const
{
  if (_updatePeriod_ms == 0) {
    UpdateUptimeAndIdleTime();
  }
  idleTime_s = _idleTime_s;
  return _uptime_s;
}

uint32_t OSState::GetMemoryInfo(uint32_t &freeMem_kB, uint32_t &availableMem_kB) const
{
  if (_updatePeriod_ms == 0) {
    UpdateMemoryInfo();
  }
  freeMem_kB = _memFree_kB;
  availableMem_kB = _memAvailable_kB;
  return _memTotal_kB;
}

const std::vector<std::string>& OSState::GetCPUTimeStats() const
{
  if (_updatePeriod_ms == 0) {
    UpdateCPUTimeStats();
  }
  return _CPUTimeStats;
}

const std::string& OSState::GetRobotName() const
{
  static const std::string name = "Vector_0000";
  return name;
}

bool OSState::IsInRecoveryMode()
{
  return false;
}

bool OSState::HasValidEMR() const
{
  return false;
}

const std::string & OSState::GetBootID()
{
  if (_bootID.empty()) {
    char buf[BUFSIZ] = "";
    size_t bufsiz = sizeof(buf);
    if (sysctlbyname("kern.bootsessionuuid", &buf, &bufsiz, NULL, 0) == 0) {
      _bootID = std::string(buf, bufsiz);
    }
    if (_bootID.empty()) {
      LOG_ERROR("OSState.GetBootID", "Unable to read boot session ID");
    }
  }
  return _bootID;
}

bool OSState::IsWallTimeSynced() const
{
  // assume mac is always synced (not really accurate... but good enough)
  return true;
}

bool OSState::HasTimezone() const
{
  // assume mac always has locale set
  return true;
}

bool OSState::IsUserSpaceSecure()
{
  return true;
}

void OSState::SetOnChargeContacts(const bool onChargeContacts) const
{
  // Do nothing
}

} // namespace Vector
} // namespace Anki
