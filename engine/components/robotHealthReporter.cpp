/**
 * File: engine/components/robotHealthReporter.cpp
 *
 * Description: Report robot & OS health events to DAS
 *
 * Copyright: Anki, Inc. 2018
 *
 **/



#include "robotHealthReporter.h"
#include "osState/osState.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"

#include <list>

#define LOG_CHANNEL "RobotHealthReporter"

namespace {
  // Have we already reported health since booting?
  // This path should point to a temporary filesystem that will be cleared by each reboot.
  const char * kOncePerBootPath = "/tmp/robot_health_once_per_boot";

  // Which filesystems do we monitor for this platform?
  #ifdef ANKI_PLATFORM_VICOS
  const std::list<std::string> kDiskInfoPaths = {"/data", "/tmp", "/run"};
  #else
  const std::list<std::string> kDiskInfoPaths = {"/tmp"};
  #endif
}

namespace Anki {
namespace Vector {

RobotHealthReporter::RobotHealthReporter() : IDependencyManagedComponent(this, RobotComponentID::RobotHealthReporter)
{

}

void RobotHealthReporter::BootInfoCheck(const OSState * osState)
{
  DEV_ASSERT(osState != nullptr, "RobotHealthReporter.BootInfoCheck.InvalidOSState");

  // Report available space on up to 4 file systems

  DASMSG(robot_boot_info, "robot.boot_info", "Sent when robot boots");

  int i = 0;
  for (auto pos = kDiskInfoPaths.begin(); pos != kDiskInfoPaths.end(); ++pos, ++i) {
    const auto & path = *pos;
    uint64_t avail_kB = 0;
    OSState::DiskInfo diskInfo;
    if (osState->GetDiskInfo(path, diskInfo)) {
      avail_kB = diskInfo.avail_kB;
    }

    // Populate s1/i1, s2/i2, etc
    switch (i) {
      case 0:
        DASMSG_SET(s1, path, "Path 1");
        DASMSG_SET(i1, avail_kB, "Available space in path 1 (kB)");
        break;
      case 1:
        DASMSG_SET(s2, path, "Path 2");
        DASMSG_SET(i2, avail_kB, "Available space in path 2(kB)");
        break;
      case 2:
        DASMSG_SET(s3, path, "Path 3");
        DASMSG_SET(i3, avail_kB, "Available space in path 3 (kB)");
        break;
      case 3:
        DASMSG_SET(s4, path, "Path 4");
        DASMSG_SET(i4, avail_kB, "Available space in path 4 (kB)");
        break;
    }
  }

  // Send the message we have populated with disk info
  DASMSG_SEND();

}

void RobotHealthReporter::WifiInfoCheck(const OSState * osState)
{
  DEV_ASSERT(osState != nullptr, "RobotHealthReporter.WifiInfoCheck.InvalidOSState");

  // Get current wifi info.
  // If we can't get wifi info, treat it as a red alert.
  OSState::WifiInfo wifiInfo;
  if (!osState->GetWifiInfo(wifiInfo)) {
    LOG_ERROR("RobotHealthReporter.WifiInfoCheck.InvalidWifiInfo", "Unable to fetch wifi info");
    wifiInfo.alert = OSState::Alert::Red;
  }

  // Has wifi crossed an alert threshold?
  if (_wifiInfo.alert != wifiInfo.alert) {
    DASMSG(robot_wifi_info, "robot.wifi_info", "Wifi status has crossed alert threshold");
    DASMSG_SET(s1, wifiInfo.alert, "Alert level");
    DASMSG_SET(i1, wifiInfo.rx_bytes, "Bytes received");
    DASMSG_SET(i2, wifiInfo.tx_bytes, "Bytes sent")
    DASMSG_SET(i3, wifiInfo.rx_errors, "Receive errors");
    DASMSG_SET(i4, wifiInfo.tx_errors, "Send errors");
    DASMSG_SEND();
    _wifiInfo = wifiInfo;
  }
}

void RobotHealthReporter::DiskInfoCheck(const OSState * osState)
{
  DEV_ASSERT(osState != nullptr, "RobotHealthReporter.DiskInfoCheck.InvalidOSState");

  for (const auto & path : kDiskInfoPaths) {
    // Get disk info for this path.
    // If we can't get disk info, treat it as a red alert.
    OSState::DiskInfo info;
    if (!osState->GetDiskInfo(path, info)) {
      LOG_ERROR("RobotHealthReporter.DiskInfoCheck.InvalidDiskInfo", "Unable to fetch disk info for %s", path.c_str());
      info.alert = OSState::Alert::Red;
    }

    // Has filesystem crossed an alert threshold?
    if (_diskInfo[path].alert != info.alert) {
      DASMSG(robot_disk_info, "robot.disk_info", "Disk status has crossed alert threshold");
      DASMSG_SET(s1, info.alert, "Alert level");
      DASMSG_SET(s2, path, "Path");
      DASMSG_SET(i1, info.total_kB, "Total space (kB)");
      DASMSG_SET(i2, info.avail_kB, "Available space (kB)");
      DASMSG_SET(i3, info.free_kB, "Free space (kB)");
      DASMSG_SET(i4, info.pressure, "Disk pressure factor");
      DASMSG_SEND();
      _diskInfo[path] = info;
    }
  }
}

void RobotHealthReporter::MemoryInfoCheck(const OSState * osState)
{
  DEV_ASSERT(osState != nullptr, "RobotHealthReporter.MemoryInfoCheck.InvalidOSState");

  // Report if memory pressure crosses alert threshold
  OSState::MemoryInfo info;
  osState->GetMemoryInfo(info);
  if (info.alert != _memoryAlert) {
    DASMSG(robot_memory_info, "robot.memory_info", "Memory pressure has crossed alert threshold");
    DASMSG_SET(s1, info.alert, "Alert level");
    DASMSG_SET(i1, info.totalMem_kB, "Total memory (kB)");
    DASMSG_SET(i2, info.availMem_kB, "Available memory (kB)");
    DASMSG_SET(i3, info.freeMem_kB, "Free memory (kB)");
    DASMSG_SET(i4, info.pressure, "Memory pressure factor");
    DASMSG_SEND();
   _memoryAlert = info.alert;
  }
}

void RobotHealthReporter::CPUInfoCheck(const OSState * osState)
{
  DEV_ASSERT(osState != nullptr, "RobotHealthReporter.CPUInfoCheck.InvalidOSState");

  // Report if cpu frequency has changed
  const uint32_t cpuFreq_kHz = osState->GetCPUFreq_kHz();
  if (cpuFreq_kHz != _cpuFreq_kHz) {
    DASMSG(robot_cpu_info, "robot.cpu_info", "CPU frequency has changed");
    DASMSG_SET(i1, cpuFreq_kHz, "New CPU frequency");
    DASMSG_SET(i2, _cpuFreq_kHz, "Old CPU frequency");
    DASMSG_SEND();
    _cpuFreq_kHz = cpuFreq_kHz;
  }
}

void RobotHealthReporter::OncePerBootCheck()
{
  LOG_DEBUG("RobotHealthReport.OncePerBootCheck", "Run once-per-boot check");
  if (Util::FileUtils::FileExists(kOncePerBootPath)) {
    LOG_DEBUG("RobotHealthReport.OncePerBootCheck", "Already reported");
    return;
  }

  // Report stuff that needs to be checked once per boot
  OSState * osState = OSState::getInstance();
  BootInfoCheck(osState);

  // Prevent report from running again until reboot
  Util::FileUtils::TouchFile(kOncePerBootPath);
}

void RobotHealthReporter::OncePerStartupCheck()
{
  // TBD
  // Report stuff that needs to be checked once per startup
}

void RobotHealthReporter::OncePerMinuteCheck()
{
  // Report stuff that needs to be checked once per minute
  const OSState * osState = OSState::getInstance();
  WifiInfoCheck(osState);
  DiskInfoCheck(osState);
}

void RobotHealthReporter::OncePerSecondCheck()
{
  // Report stuff that needs to be checked once per second
  const auto * osState = OSState::getInstance();
  MemoryInfoCheck(osState);
  CPUInfoCheck(osState);
}

//
//
// Called once at robot init
//
void RobotHealthReporter::InitDependent(Robot * robot, const DependencyManager & dependencyManager)
{
  // Nothing to do here?
}

//
// Called once per robot tick
//
void RobotHealthReporter::UpdateDependent(const DependencyManager & dependencyManager)
{
  if (_once_per_boot) {
    _once_per_boot = false;
    OncePerBootCheck();
  }

  if (_once_per_startup) {
    _once_per_startup = false;
    OncePerStartupCheck();
  }

  // Maybe use setitimer instead of polling system clock?
  const auto now = std::chrono::steady_clock::now();

  {
    // Has a minute elapsed since last once-per-minute check?
    const auto elapsed = (now - _once_per_minute_time);
    const auto elapsed_minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
    if (elapsed_minutes.count() >= 1) {
      OncePerMinuteCheck();
      _once_per_minute_time = now;
    }
  }

  {
    // Has a second elapsed since last once-per-second check?
    const auto elapsed = (now - _once_per_second_time);
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);
    if (elapsed_seconds.count() >= 1) {
      OncePerSecondCheck();
      _once_per_second_time = now;
    }
  }

}

void RobotHealthReporter::OnSetLocale(const std::string & locale)
{
  if (_locale != locale) {
    DASMSG(robot_locale, "robot.locale", "Robot locale");
    DASMSG_SET(s1, locale, "New locale");
    DASMSG_SET(s2, _locale, "Old locale");
    DASMSG_SEND();
    _locale = locale;
  }
}

void RobotHealthReporter::OnSetTimeZone(const std::string & timezone)
{
  if (_timezone != timezone) {
    DASMSG(robot_timezone, "robot.timezone", "Robot timezone");
    DASMSG_SET(s1, timezone, "New timezone");
    DASMSG_SET(s2, _timezone, "Old timezone");
    DASMSG_SEND();
    _timezone = timezone;
  }
}

} // end namespace Vector
} // end namespace Anki
