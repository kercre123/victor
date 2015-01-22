//
//  CozmoEngineWrapperTypes.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/12/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef CozmoVision_CozmoEngineWrapperTypes_h
#define CozmoVision_CozmoEngineWrapperTypes_h
@class CozmoEngineWrapper;

typedef NS_ENUM (int, CozmoEngineRunState) {
  CozmoEngineRunStateNone = 0,           // Nothing is running
  CozmoEngineRunStateCommsReady,         // Comms ready, searching for robot
  CozmoEngineRunStateRobotConnected,     // Robot is connected, we are ready to start
  CozmoEngineRunStateRunning             // Running basedtatoin
};

@protocol CozmoEngineHeartbeatListener <NSObject>

- (void)cozmoEngineWrapperHeartbeat:(CozmoEngineWrapper*)cozmoEngine;

@end



#endif
