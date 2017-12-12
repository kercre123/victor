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

#include "cozmoAnim/osState.h"

namespace Anki {
namespace Cozmo {
  
  namespace{

  } // namespace
  
  OSState::OSState()
  {
    
  }
  
  void OSState::Update()
  {
  }
  
  uint32_t OSState::GetCPUFreq_kHz() const
  {
    return kNominalCPUFreq_kHz;
  }
  
  bool OSState::IsThermalThrottling() const
  {
    return false;
  }
  
} // namespace Cozmo
} // namespace Anki

