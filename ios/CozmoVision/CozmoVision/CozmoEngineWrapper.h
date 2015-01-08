//
//  CozmoEngineWrapper.h
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CozmoEngineWrapperTypes.h"

#ifdef __cplusplus
#import <anki/vision/basestation/image.h>
#endif

@class CozmoOperator;

// IP Address of wherever the robot's advertising service is running
#define DEFAULT_ADVERTISING_HOST_IP     "127.0.0.1"
// Tick the cozmoeEngine rate, ticks per second
#define DEFAULT_HEARTBEAT_RATE          40.0
// IP Address of wherever the visualization (Webots) is running
#define DEFAULT_VIZ_HOST_IP             "192.168.19.238"

@interface CozmoEngineWrapper : NSObject

@property (readonly) CozmoEngineRunState runState;

// Return the CozmoEngineWrapper instance held onto by the AppDelegate
+ (instancetype)defaultEngine;

// Config Parameters
// Can only set config properties when the runState == stopped
- (NSString*)hostAdvertisingIP;
- (BOOL)setHostAdvertisingIP:(NSString*)advertisingIP;

- (NSString*)vizIP;
- (BOOL)setVizIP:(NSString*)vizIP;

- (double)heartbeatRate;  // Run loop frequency (Ticks per second)
- (BOOL)setHeartbeatRate:(NSTimeInterval)rate;

- (int)numRobotsToWaitFor;
- (BOOL)setNumRobotsToWaitFor:(int)N;

- (int)numUiDevicesToWaitFor;
- (BOOL)setNumUiDevicesToWaitFor:(int)N;

// Listeners
// CozmoeEngine Heartbeat
- (void)addListener:(id<CozmoEngineHeartbeatListener>)listener;
- (void)removeListener:(id<CozmoEngineHeartbeatListener>)listener;

@property (readonly) CozmoOperator *cozmoOperator;

// Finite control
- (BOOL)start:(BOOL)asHost;

// Change engine's run state
// Wait for 1 or more robots to connect before calling
- (void)stop;

- (UIImage*)imageFrameWtihRobotId:(uint8_t)robotId;

- (NSArray*)boundingBoxesObservedByRobotId:(uint8_t)robotId;


-(BOOL)checkDeviceVisionMailbox:(CGRect*)markerBBoxOut :(int*)markerTypeOut;

#ifdef __cplusplus
-(void)processDeviceImage:(Anki::Vision::Image&)image;
#endif

-(BOOL) wasLastDeviceImageProcessed;

@end
