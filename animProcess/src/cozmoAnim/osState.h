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

#include <stdint.h>
#include "util/singleton/dynamicSingleton.h"

namespace Anki {
namespace Cozmo {
  
class OSState : public Util::DynamicSingleton<OSState>
{
  ANKIUTIL_FRIEND_SINGLETON(OSState);
    
public:
  void Update();
  
  bool IsThermalThrottling() const;
  uint32_t GetCPUFreq_kHz() const;
  
private:
  // private ctor
  OSState();
  
  const uint32_t kNominalCPUFreq_kHz = 800000;
  uint32_t _cpuFreq_kHz;

}; // class OSState
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Victor_OSState_H__ */
