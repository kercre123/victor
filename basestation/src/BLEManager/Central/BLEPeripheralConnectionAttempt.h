//
//  BLEPeripheralConnectionAttempt.h
//  BLEManager
//
//  Created by Mark Pauley on 3/29/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>

@class CBPeripheral;

typedef NS_ENUM(NSUInteger, BLEPeripheralConnectionState) {
  BLEPeripheralConnectionStatePending,
  BLEPeripheralConnectionStateConnecting,
  BLEPeripheralConnectionStateConnected,
  BLEPeripheralConnectionStateDiscoveringServices,  
  BLEPeripheralConnectionStateDiscoveringCharacteristics,
  BLEPeripheralConnectionStateCancelled,
};

@interface BLEPeripheralConnectionAttempt : NSObject
@property (readwrite, nonatomic) NSUInteger numAttempts;
@property (strong, nonatomic) CBPeripheral* peripheral;
@property (readwrite, nonatomic) CFAbsoluteTime timeStamp;
@property (readwrite, nonatomic) BLEPeripheralConnectionState state;
@end
