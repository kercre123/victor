//
//  BLEVehicleMessage.m
//  BLEManager
//
//  Created by Mark Pauley on 4/8/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import "BLEManager/Vehicle/BLEVehicleMessage.h"

@implementation BLEVehicleMessage

@synthesize vehicleID;
@synthesize uniqueVehicleID;
@synthesize msgData;
@synthesize timestamp;

- (id) initWithData:(NSData *)data
{
  self = [super init];
  if (self) {
    self.msgData = data;
	}
  
  return self;
}


- (void) dealloc
{
	if (self.msgData) {
  }
}

-(uint8_t)messageSize {
  if(self.msgData.length > 0) {
    return ((uint8_t*)self.msgData.bytes)[0];
  }
  return 0;
}

-(uint8_t)messageType {
  if(self.messageSize > 0) {
    return ((uint8_t*)self.msgData.bytes)[1];
  }
  return 0;
}

@end
