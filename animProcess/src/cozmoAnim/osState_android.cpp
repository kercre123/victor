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

#include "cozmoAnim/osState.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include <fstream>

namespace Anki {
namespace Cozmo {
  
  namespace{
    
    const char* kCPUFreqFile = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
    //const char* kTemperatureFile = "/sys/class/thermal/thermal_zone0/temp";

    // How often state variables are updated
    uint32_t _updatePeriod_ms = 1000;
    uint32_t _lastUpdateTime_ms = 0;

  } // namespace
  
  OSState::OSState()
  : _cpuFreq_kHz(kNominalCPUFreq_kHz)
  {
    
  }
  
  void OSState::Update()
  {
    const uint32_t now_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    if (now_ms - _lastUpdateTime_ms > _updatePeriod_ms) {
      
      // Update cpu freq
      std::ifstream cpuFile(kCPUFreqFile, std::ifstream::in);
      cpuFile >> _cpuFreq_kHz;
      cpuFile.close();
      
      _lastUpdateTime_ms = now_ms;
    }
  }
  
  uint32_t OSState::GetCPUFreq_kHz() const
  {
    return _cpuFreq_kHz;
  }
  
  bool OSState::IsThermalThrottling() const
  {
    return (_cpuFreq_kHz < kNominalCPUFreq_kHz);
  }
  
} // namespace Cozmo
} // namespace Anki

