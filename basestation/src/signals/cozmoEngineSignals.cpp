//
//  CozmoEngineSignals.cpp
//
//  Description: Signals that are emitted from within CozmoGame that other parts of CozmoGame may listen for.
//               Primarily used to trigger signals based on incoming U2G messages.
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/signals/CozmoEngineSignals.h"

// Macro for creating signal defintion
// e.g.
// DEF_SIGNAL(CozmoEngineSignals, RobotConnect) creates
//
//  static CozmoEngineSignals::RobotConnectSignal _RobotConnectSignal;
//  CozmoEngineSignals::RobotConnectSignal& CozmoEngineSignals::GetRobotConnectSignal() { return _RobotConnectSignal; }
//
#define DEF_SIGNAL(__CLASSNAME__,__SIGNAME__) \
static __CLASSNAME__::__SIGNAME__##Signal _##__SIGNAME__##Signal; \
__CLASSNAME__::__SIGNAME__##Signal& __CLASSNAME__::Get##__SIGNAME__##Signal() { return _##__SIGNAME__##Signal; } \


namespace Anki {
  namespace Cozmo {

    DEF_SIGNAL(CozmoEngineSignals,RobotConnect)
    DEF_SIGNAL(CozmoEngineSignals,RobotAvailable)
    DEF_SIGNAL(CozmoEngineSignals,UiDeviceAvailable)
    
    DEF_SIGNAL(CozmoEngineSignals,PlaySoundForRobot)
    DEF_SIGNAL(CozmoEngineSignals,StopSoundForRobot)
    
    DEF_SIGNAL(CozmoEngineSignals,DeviceDetectedVisionMarker)
    DEF_SIGNAL(CozmoEngineSignals,RobotObservedObject)
    
  } // namespace Cozmo
} // namespace Anki
