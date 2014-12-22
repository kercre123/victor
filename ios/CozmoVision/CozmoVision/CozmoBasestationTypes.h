//
//  CozmoBasestationTypes.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/12/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef CozmoVision_CozmoBasestationTypes_h
#define CozmoVision_CozmoBasestationTypes_h
@class CozmoBasestation;

typedef NS_ENUM (int, CozmoBasestationRunState) {
  CozmoBasestationRunStateNone = 0,           // Nothing is running
  CozmoBasestationRunStateCommsReady,         // Comms ready, searching for robot
  CozmoBasestationRunStateRobotConnected,     // Robot is connected, we are ready to start basestation
  CozmoBasestationRunStateRunning             // Running basedtatoin
};

@protocol CozmoBasestationHeartbeatListener <NSObject>

- (void)cozmoBasestationHearbeat:(CozmoBasestation*)basestation;

@end

#endif
