//
//  BLECentralMultiplexer.h
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h>

extern NSString * const BLECentralMultiplexerIsWedgedNotification;

@class BLECentralServiceDescription;

@interface BLECentralMultiplexer : NSObject

+(BLECentralMultiplexer*)sharedInstance;

-(id)initWithCentralManager:(CBCentralManager*)centralManager;

// service registration
-(void)registerService:(BLECentralServiceDescription*)serviceDescription;
-(void)cancelService:(BLECentralServiceDescription*)serviceDescription;
-(BLECentralServiceDescription*)serviceForUUID:(CBUUID*)serviceID;

// scanning
-(void)startScanningForService:(BLECentralServiceDescription*)serviceDescription;
-(void)stopScanningForService:(BLECentralServiceDescription*)serviceDescription;
@property (nonatomic,readonly) BOOL isScanning;

// peripheral connection / disconnection
-(void)connectPeripheral:(CBPeripheral*)peripheral withService:(BLECentralServiceDescription*)service;
-(void)disconnectPeripheral:(CBPeripheral*)peripheral withService:(BLECentralServiceDescription*)service;

// stop scanning and drop all connections
-(void)reset;
@end
