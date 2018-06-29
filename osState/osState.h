/**
 * File: OSState.h
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

#ifndef __Victor_OSState_H__
#define __Victor_OSState_H__

#include "coretech/common/shared/types.h"
#include "util/singleton/dynamicSingleton.h"
#include "json/json.h"

#include <functional>
#include <string>

// Forward declarations
namespace webots {
  class Supervisor;
}
namespace Anki {
  namespace Victor {
    class DASManager;
  }
}

namespace Anki {
namespace Cozmo {

class OSState : public Util::DynamicSingleton<OSState>
{
  ANKIUTIL_FRIEND_SINGLETON(OSState);

public:

~OSState();

#ifdef SIMULATOR
  // Assign Webots supervisor
  // Webots processes must do this before creating OSState for the first time.
  // Unit test processes must call SetSupervisor(nullptr) to run without a supervisor.
  static void SetSupervisor(webots::Supervisor *sup);
#endif

  void Update();

  RobotID_t GetRobotID() const;

  // Set how often state should be updated.
  // Affects how often the freq and temperature is updated.
  // Default is 0 ms which means never update.
  // You should leave this at zero only if you don't
  // ever care about CPU freq and temperature.
  void SetUpdatePeriod(uint32_t milliseconds);

  void SendToWebVizCallback(const std::function<void(const Json::Value&)>& callback);

  // Returns true if CPU frequency falls below kNominalCPUFreq_kHz
  bool IsCPUThrottling() const;

  // Returns current CPU frequency
  uint32_t GetCPUFreq_kHz() const;

  // Returns temperature in Celsius
  uint32_t GetTemperature_C() const;

  // Returns uptime (and idle time) in seconds
  float GetUptimeAndIdleTime(float &idleTime_s) const;

  // Returns total and free/available memory in kB
  uint32_t GetMemoryInfo(uint32_t &freeMem_kB, uint32_t &availableMem_kB) const;

  // Returns data about CPU times
  const std::vector<std::string>& GetCPUTimeStats() const;

  // Returns our ip address
  const std::string& GetIPAddress(bool update = false);

  // Returns the SSID of the connected wifi network
  const std::string& GetSSID(bool update = false);

  // Returns our mac address
  std::string GetMACAddress() const;

  uint64_t GetWifiTxBytes() const;
  uint64_t GetWifiRxBytes() const;

  // Returns the ESN (electronic serial number) as a u32
  u32 GetSerialNumber()
  {
    const std::string& serialNum = GetSerialNumberAsString();
    if(!serialNum.empty())
    {
      return static_cast<u32>(std::stoul(serialNum, nullptr, 16));
    }

    return 0;
  }

  // Returns the ESN (electronic serial number) as a string
  const std::string& GetSerialNumberAsString();

  // Returns the os build version (time of build)
  const std::string& GetOSBuildVersion();

  // Returns "major.minor.build" for reporting to DAS
  const std::string& GetRobotVersion();

  const std::string& GetBuildSha();

  // Returns the semi-unique name of this robot, Vector_XYXY
  // Where X is a letter and Y is a digit
  // The name can change over the lifetime of the robot
  const std::string& GetRobotName() const;

  // Return GUID string generated each time robot boots
  const std::string& GetBootID();

  // Returns whether or not the robot has booted in recovery mode
  // which is done by holding the backpack button down for ~12 seconds
  // while robot is on charger
  bool IsInRecoveryMode();

  // True if we've synced time with a time server
  bool IsWallTimeSynced() const;

  // True if timezone is set (and therefore we can get local time)
  bool HasTimezone() const;

  // True if user space is secure
  bool IsUserSpaceSecure();
  
  // For the engine to let the OS State know if we are on/off the charge contacts
  void SetOnChargeContacts(const bool onChargeContacts) const;

protected:
   // Return true if robot has a valid EMR.
   // This function is "off limits" to normal robot services
   // but allows vic-dasmgr to check for ESN without crashing.
  friend class Anki::Victor::DASManager;
  bool HasValidEMR() const;

private:
  // private ctor
  OSState();

  void UpdateWifiInfo();

  // Reads the current CPU frequency
  void UpdateCPUFreq_kHz() const;

  // Reads the temperature in Celsius
  void UpdateTemperature_C() const;

  // Reads the battery voltage in microvolts
  void UpdateBatteryVoltage_uV() const;

  // Reads uptime (and idle time) data
  void UpdateUptimeAndIdleTime() const;

  // Reads memory info data
  void UpdateMemoryInfo() const;

  // Reads CPU times data
  void UpdateCPUTimeStats() const;

  uint32_t kNominalCPUFreq_kHz = 800000;

  std::string _ipAddress       = "";
  std::string _ssid            = "";
  std::string _serialNumString = "";
  std::string _osBuildVersion  = "";
  std::string _robotVersion    = "";
  std::string _buildSha        = "";
  std::string _bootID          = "";
  bool        _isUserSpaceSecure = false;
  
}; // class OSState

} // namespace Cozmo
} // namespace Anki

#endif /* __Victor_OSState_H__ */
