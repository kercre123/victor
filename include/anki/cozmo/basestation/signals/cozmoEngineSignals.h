//
//  CozmoEngineSignals.h
//
//  Description: Signals that are emitted from within CozmoEngine that CozmoGame may listen for.
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __COZMO_ENGINE_SIGNALS__
#define __COZMO_ENGINE_SIGNALS__

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
      
    class CozmoEngineSignals {
      
    public:

      //DECL_SIGNAL(RobotConnect, RobotID_t robotID, bool successful)
      DECL_SIGNAL(RobotConnect, RobotID_t robotID, bool successful)
      DECL_SIGNAL(RobotAvailable, RobotID_t robotID)
      DECL_SIGNAL(UiDeviceAvailable, UserDeviceID_t deviceID)
      
      
      // Robot signals
      DECL_SIGNAL(PlaySoundForRobot, RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume)
      DECL_SIGNAL(StopSoundForRobot, RobotID_t robotID)
      
      
      // Vision signals
      DECL_SIGNAL(RobotImageAvailable,
                  uint8_t robotID)
      DECL_SIGNAL(DeviceDetectedVisionMarker,
                  uint8_t engineID, uint32_t markerType,
                  float x_upperLeft,  float y_upperLeft,
                  float x_lowerLeft,  float y_lowerLeft,
                  float x_upperRight, float y_upperRight,
                  float x_lowerRight, float y_lowerRight)
      DECL_SIGNAL(RobotObservedObject,
                  uint8_t robotID, uint32_t objectID,
                  float x_upperLeft,  float y_upperLeft,
                  float width,  float height)
    };
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // __COZMO_ENGINE_SIGNALS__