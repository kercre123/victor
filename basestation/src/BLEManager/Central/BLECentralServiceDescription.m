//
//  BLECentralServiceDescription.m
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import "BLEManager/Central/BLECentralServiceDescription.h"

@implementation BLECentralServiceDescription

@synthesize delegate = _delegate;
@synthesize serviceID = _serviceID;
@synthesize characteristicIDs = _characteristicIDs;
@synthesize name = _name;


-(BOOL)isEqual:(id)object {
  if(object == self) {
    return YES;
  }
  
  if([object isKindOfClass:[BLECentralServiceDescription class]]) {
    BLECentralServiceDescription* other = (BLECentralServiceDescription*)object;
    return (self.delegate == other.delegate
            && [self.serviceID isEqual:other.serviceID]
            && [self.characteristicIDs isEqual:other.characteristicIDs]);
  }
  return NO;
}

-(NSUInteger)hash {
  NSUInteger characteristicHashAccumulator = 0;
  for(CBUUID* characteristicUUID in self.characteristicIDs) {
    characteristicHashAccumulator ^= [characteristicUUID hash];
  }
  return ([self.delegate hash] ^ [self.serviceID hash] ^ characteristicHashAccumulator);
}

-(NSString*)description {
  return [NSString stringWithFormat:@"<BLECentralServiceDescription name=%@>", self.name];
}

@end
