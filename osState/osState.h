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

  // Reads the current CPU frequency and returns it
  uint32_t UpdateCPUFreq_kHz() const;

  // Reads the temperature in Celsius and returns it
  uint32_t UpdateTemperature_C() const;

  // Returns current CPU frequency
  // Asserts if update rate is 0
  uint32_t GetCPUFreq_kHz() const;
  
  // Returns temperature in Celsius
  // Asserts if update rate is 0
  uint32_t GetTemperature_C() const;

  // Returns our ip address
  const std::string& GetIPAddress(bool update = false);

  // Returns the SSID of the connected wifi network
  const std::string& GetSSID(bool update = false);

  // Returns our mac address
  std::string GetMACAddress() const;

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

  // Returns the semi-unique name of this robot, Vector_XYXY
  // Where X is a letter and Y is a digit
  // The name can change over the lifetime of the robot
  const std::string& GetRobotName() const;
  
  // Returns whether or not the robot has booted in recovery mode
  // which is done by holding the backpack button down for ~12 seconds
  // while robot is on charger
  bool IsInRecoveryMode();

private:
  // private ctor
  OSState();

  void UpdateWifiInfo();
  
  uint32_t kNominalCPUFreq_kHz = 800000;

  std::string _ipAddress       = "";
  std::string _ssid            = "";
  std::string _serialNumString = "";
  std::string _osBuildVersion  = "";

}; // class OSState
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Victor_OSState_H__ */
