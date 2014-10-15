//
//  BLEVehicleMessage.h
//  BLEManager
//
//  Created by Mark Pauley on 4/8/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef uint8_t BLEVehicleMessageType;
typedef uint8_t BLEVehicleMessageSize;

#define BLEVehicleMessageTypeMax 255

#define BLEVehicleMessageMaxMessageLength 20

@interface BLEVehicleMessage : NSObject  {
  UInt64 _timestamp;
}

// Message addressing.
@property (assign) NSInteger          vehicleID; // between 1 and 255, used by Basestation
@property (assign) UInt64             uniqueVehicleID; //mfgID
@property (assign) UInt64             timestamp; // in nanoseconds

// Raw message bytes as received from the vehicle.
@property (strong, nonatomic) NSData  *msgData;

// Helpers for inspection into the message.
@property (readonly) BLEVehicleMessageSize  messageSize;
@property (readonly) BLEVehicleMessageType  messageType;

- (id) initWithData:(NSData *)data;

@end