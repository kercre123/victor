//
//  CozmoGameSignals.cpp
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/game/signals/CozmoGameSignals.h"

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

    DEF_SIGNAL(CozmoGameSignals,ConnectToRobot)
    DEF_SIGNAL(CozmoGameSignals,ConnectToUiDevice)
    DEF_SIGNAL(CozmoGameSignals,UiDeviceAvailable)
    DEF_SIGNAL(CozmoGameSignals,UiDeviceConnected)
    
  } // namespace Cozmo
} // namespace Anki
