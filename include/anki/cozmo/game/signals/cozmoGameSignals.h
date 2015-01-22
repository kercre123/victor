//
//  CozmoGameSignals.h
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __COZMO_GAME_SIGNALS__
#define __COZMO_GAME_SIGNALS__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/utils/signals/simpleSignal.hpp"


// Macro for declaring signal type and accessor function
// e.g. DECL_SIGNAL(RobotConnect, RobotID_t robotID, bool successful) creates
//
//  using RobotConnectSignal = Signal::Signal<void (RobotID_t robotID, bool successful)>;
//  static RobotConnectSignal& GetRobotConnectSignal();
//
//  A listener may subscribe to a signal with a callback function like so.
//
//      auto cbRobotConnectSignal = [this](RobotID_t robotID, bool successful) {
//        this->HandleRobotConnectSignal(robotID, successful);
//      };
//      _signalHandles.emplace_back( CozmoEngineSignals::GetRobotConnectSignal().ScopedSubscribe(cbRobotConnectSignal));
//
//      where _signalHandles is a container for the SmartHandle that ScopedSubscribe() returns.
//      In this case, _signalHandles should be a member of the same class as HandleRobotConnectSignal().
//      See simpleSignal.hpp for more details.
//
#define DECL_SIGNAL(__SIGNAME__, ...) \
using __SIGNAME__##Signal = Signal::Signal<void (__VA_ARGS__)>; \
static __SIGNAME__##Signal& Get##__SIGNAME__##Signal();

namespace Anki {
  namespace Cozmo {

    class CozmoGameSignals {
    public:
    
      // Tell game host to connect to robot
      DECL_SIGNAL(ConnectToRobot, RobotID_t robotID)
    
      // Tell game host to connect to a UI device
      DECL_SIGNAL(ConnectToUiDevice, UserDeviceID_t deviceID)
      
      // Signal that a UI device is available for connection
      DECL_SIGNAL(UiDeviceAvailable, UserDeviceID_t deviceID)
      
      // Signal that a UI device actually connected
      DECL_SIGNAL(UiDeviceConnected, UserDeviceID_t deviceID, bool successful)
      
    };
    
  } // namespace Cozmo
} // namespace Anki

#endif // __COZMO_GAME_SIGNALS__