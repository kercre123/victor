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

#include <fstream>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using osState_android.cpp
#endif

namespace Anki {
namespace Cozmo {
  
  namespace{

    std::ifstream _cpuFile;
    std::ifstream _tempFile;

    const char* kNominalCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
    const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
    const char* kTemperatureFile = "/sys/devices/virtual/thermal/thermal_zone8/temp";

    // System vars
    uint32_t _cpuFreq_kHz; // CPU freq
    uint32_t _cpuTemp_mC;  // Temperature in milli-Celsius

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
    _cpuTemp_mC = 0;

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
        _cpuFile.seekg(0, _cpuFile.beg);
        _cpuFile >> _cpuFreq_kHz;
        
        // Update temperature reading
        _tempFile.seekg(0, _tempFile.beg);
        _tempFile >> _cpuTemp_mC;

        _lastUpdateTime_ms = now_ms;
      }
    }
  }
  
  void OSState::SetUpdatePeriod(uint32_t milliseconds)
  {
    _updatePeriod_ms = milliseconds;
  }

  uint32_t OSState::GetCPUFreq_kHz() const
  {
    DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetCPUFreq_kHz.ZeroUpdate");
    return _cpuFreq_kHz;
  }
  
  bool OSState::IsThermalThrottling() const
  {
    DEV_ASSERT(_updatePeriod_ms != 0, "OSState.IsThermalThrottling.ZeroUpdate");
    return (_cpuFreq_kHz < kNominalCPUFreq_kHz);
  }
  
  uint32_t OSState::GetTemperature_mC() const
  {
    DEV_ASSERT(_updatePeriod_ms != 0, "OSState.GetTemperature_mC.ZeroUpdate");
    return _cpuTemp_mC;
  }

} // namespace Cozmo
} // namespace Anki

