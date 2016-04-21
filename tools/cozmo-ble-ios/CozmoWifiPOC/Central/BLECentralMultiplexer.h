//
//  BLECentralMultiplexer.h
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

extern NSString * const BLECentralMultiplexerIsWedgedNotification;

@class BLECentralServiceDescription;

@interface BLECentralMultiplexer : NSObject

-(id)initWithCentralManager:(CBCentralManager *)centralManager
                      queue:(dispatch_queue_t)queue;

// service registration
-(void)registerService:(BLECentralServiceDescription*)serviceDescription;
-(void)cancelService:(BLECentralServiceDescription*)serviceDescription;
-(BLECentralServiceDescription*)serviceForUUID:(CBUUID*)serviceID;

// scanning

typedef NS_ENUM(NSUInteger, BLECentralMultiplexerScanningState) {
  BLECentralMultiplexerScanningStateStopped, // no scanning services
  BLECentralMultiplexerScanningStateScanning, // 1 or more scanning services
  BLECentralMultiplexerScanningStatePaused,  // CoreBluetooth scanning disabled
};

-(void)startScanningForService:(BLECentralServiceDescription*)serviceDescription;
-(void)stopScanningForService:(BLECentralServiceDescription*)serviceDescription;

// Return YES if the service is registered for scanning, otherwise NO.
-(BOOL)isScanningForService:(BLECentralServiceDescription*)serviceDescription;

// Pause BLE scanning for any services that are currently scanning.
// Note that 'pause' is intended to reflect a state where we stop
// scanning in the underlying CoreBluetooth layer, but BLECentralMultiplexer clients
// are not directly notified that scanning has stopped.  They will simply
// not receive advertisement updates during the pause state.
// Existing advertisements will not expire while scanning
// is paused, and will be automatically refreshed when scanning resumes.
// Calling pauseScanning has no effect if scanning is already paused
// or stopped.
-(void)pauseScanning;

// Resume BLE scanning for any services that were paused.
// Upon resuming scanning from the pause state, existing advertisements
// are automatically refreshed so that they will not expire immediately
// upon resume.
// Calling resumeScanning has no effect if scanning not paused.
-(void)resumeScanning;

// Returns scanning state.
@property (nonatomic,readonly) BLECentralMultiplexerScanningState scanningState;

// peripheral connection / disconnection
-(void)connectPeripheral:(CBPeripheral*)peripheral
             withService:(BLECentralServiceDescription*)service;

-(void)disconnectPeripheral:(CBPeripheral*)peripheral
                withService:(BLECentralServiceDescription*)service;

// stop scanning and drop all connections
-(void)reset;
@end

@interface BLECentralMultiplexer (TestSupport)
@property (nonatomic, assign) BOOL debugTimeoutNextConnectionAttempt;
@end
