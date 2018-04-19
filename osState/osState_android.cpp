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
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <linux/wireless.h>

#include <fstream>
#include <array>
#include <stdlib.h>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using osState_android.cpp
#endif

namespace Anki {
namespace Cozmo {
  
namespace {

  std::ifstream _cpuFile;
  std::ifstream _tempFile;

  const char* kNominalCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
  const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
  const char* kTemperatureFile = "/sys/devices/virtual/thermal/thermal_zone3/temp";
  const char* kMACAddressFile = "/sys/class/net/wlan0/address";
  const char* kRecoveryModeFile = "/data/unbrick";
  
  // System vars
  uint32_t _cpuFreq_kHz; // CPU freq
  uint32_t _cpuTemp_C;   // Temperature in Celsius

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
      _cpuFreq_kHz = UpdateCPUFreq_kHz();

      // Update temperature reading
      _cpuTemp_C = UpdateTemperature_C();

      _lastUpdateTime_ms = now_ms;
    }
  }
}

void OSState::SetUpdatePeriod(uint32_t milliseconds)
{
  _updatePeriod_ms = milliseconds;
}

uint32_t OSState::UpdateCPUFreq_kHz() const
{
  // Update cpu freq
  uint32_t cpuFreq_kHz;
  _cpuFile.seekg(0, _cpuFile.beg);
  _cpuFile >> cpuFreq_kHz;
  return cpuFreq_kHz;
}
      
uint32_t OSState::UpdateTemperature_C() const
{
  // Update temperature reading
  uint32_t cpuTemp_C;
  _tempFile.seekg(0, _tempFile.beg);
  _tempFile >> cpuTemp_C;
  return cpuTemp_C;
}

uint32_t OSState::GetCPUFreq_kHz() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetCPUFreq_kHz.ZeroUpdate");
  return _cpuFreq_kHz;
}


bool OSState::IsCPUThrottling() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsCPUThrottling.ZeroUpdate");
  return (_cpuFreq_kHz < kNominalCPUFreq_kHz);
}

uint32_t OSState::GetTemperature_C() const
{
  DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetTemperature_C.ZeroUpdate");
  return _cpuTemp_C;
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
    _osBuildVersion = GetProperty("ro.build.display.id");
  }
  
  return _osBuildVersion;
}

std::string OSState::GetRobotName() const
{
  static std::string name = GetProperty("persist.anki.robot.name");
  return (name.empty() ? "Vector_0000" : name);
}
  
void OSState::UpdateWifiInfo()
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

  struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
  _ipAddress = std::string(inet_ntoa(ipaddr->sin_addr));

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

bool OSState::IsInRecoveryMode()
{
  return Util::FileUtils::FileExists(kRecoveryModeFile);
}

} // namespace Cozmo
} // namespace Anki

