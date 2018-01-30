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

  // Returns true if cpu freq falls below kNominalCPUFreq_kHz
  // Asserts if update rate is 0
  bool IsThermalThrottling() const;

  // Returns current CPU frequency
  // Asserts if update rate is 0
  uint32_t GetCPUFreq_kHz() const;
  
  // Returns temperature in milli-Celsius
  // Asserts if update rate is 0
  uint32_t GetTemperature_mC() const;

  // Returns our ip address
  const std::string& GetIPAddress()
  {
    if(_ipAddress.empty())
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

  u32 GetOSBuildNumber();

private:
  // private ctor
  OSState();

  std::string GetIPAddressInternal();

  // Executes the provided command and returns the output as a string
  std::string ExecCommand(const char* cmd);
  
  uint32_t kNominalCPUFreq_kHz = 800000;

  std::string _ipAddress       = "";
  std::string _serialNumString = "";
  u32         _osBuildNum      = 0;

}; // class OSState
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Victor_OSState_H__ */
