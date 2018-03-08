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

#include <string>

// Forward declaration
namespace webots {
  class Supervisor;
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

  // Returns true if CPU frequncy falls below kNominalCPUFreq_kHz
  bool IsCPUThrottling() const;

  // Returns current CPU frequency
  uint32_t GetCPUFreq_kHz() const;
  
  // Returns temperature in Celsius
  uint32_t GetTemperature_C() const;

  // Returns battery charge in volts
  uint32_t GetBatteryVoltage_uV() const;

  // Returns uptime (and idle time) in seconds
  float GetUptimeAndIdleTime(float &idleTime_s) const;

  // Returns total and free memory in kB
  uint32_t GetMemoryInfo(uint32_t &freeMem_kB) const;

  // Returns data about CPU times
  const std::vector<std::string>& GetCPUTimeStats() const;

  // Returns our ip address
  const std::string& GetIPAddress(bool update = false)
  {
    if(_ipAddress.empty() || update)
    {
      _ipAddress = GetIPAddressInternal();
    }

    return _ipAddress;
  }

  u32 GetSerialNumber()
  {
    const std::string& serialNum = GetSerialNumberAsString();
    if(!serialNum.empty())
    {
      return static_cast<u32>(std::stoul(serialNum, nullptr, 16));
    }

    return 0;
  }

  const std::string& GetSerialNumberAsString();

  const std::string& GetOSBuildVersion();
  
  const std::string& GetBuildSha();

private:
  // private ctor
  OSState();

  std::string GetIPAddressInternal();
  
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
  std::string _serialNumString = "";
  std::string _osBuildVersion  = "";
  std::string _buildSha        = "";

}; // class OSState
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Victor_OSState_H__ */
