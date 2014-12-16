//
//  CozmoBasestation.h
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CozmoBasestationTypes.h"


// IP Address of wherever the robot's advertising service is running
#define DEFAULT_ADVERTISING_HOST_IP     "192.168.19.227"
// IP Address of wherever the Basestation service is running
#define DEFAULT_BASESTATION_IP          "127.0.0.1"
// Tick the basestation rate, ticks per second
#define DEFAULT_HEARTBEAT_RATE          40.0

@interface CozmoBasestation : NSObject

@property (readonly) CozmoBasestationRunState runState;

// Return the Basestation instance held onto by the AppDelegate
+ (instancetype)defaultBasestation;

// Config Parameters
// Can only set config properties when the runState == stopped
- (NSString*)hostAdvertisingIP;
- (BOOL)setHostAdvertisingIP:(NSString*)advertisingIP;

- (NSString*)basestationIP;
- (BOOL)setBasestationIP:(NSString*)basestationIP;

- (double)heartbeatRate;  // Run loop frequency (Ticks per second)
- (BOOL)setHeartbeatRate:(NSTimeInterval)rate;


// Listeners
// Basestation Heartbeat
- (void)addListener:(id<CozmoBasestationHeartbeatListener>)listener;
- (void)removeListener:(id<CozmoBasestationHeartbeatListener>)listener;


// TODO: This only returns one robot right now
- (UIImage*)imageFrameWtihRobotId:(uint8_t)robotId;



// Change Basestation State

- (BOOL)startComms;
// Change Basestation run state
// Wait for 1 or more robots to connect before calling
- (BOOL)startBasestation;
- (void)stopCommsAndBasestation;


@end
