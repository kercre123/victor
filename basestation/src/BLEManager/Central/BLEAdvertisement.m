//
//  BLEAdvertisement.m
//  BLEManager
//
//  Created by Mark Pauley on 3/29/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import "BLEManager/Central/BLEAdvertisement.h"
#import "BLEManager/Logging/BLELog.h"

#import <IOBluetooth/IOBluetooth.h>

@implementation BLEAdvertisement {
  NSMutableSet* _serviceIDs;
  float _rssiMedian;
  NSUInteger _rssiRingBufIndex;
}

@synthesize name;
@synthesize peripheral;
@synthesize timeStamp;
@synthesize discovered;
@synthesize advertisementData = _advertisementData;

@synthesize serviceIDs = _serviceIDs;

-(id)init {
  if((self = [super init])) {
    _serviceIDs = [NSMutableSet set];
    _rssiMedian = -56.0f;
    _advertisementData = [NSMutableDictionary dictionary];
  }
  return self;
}

#define anki_fsignum(val) (((val) < 0.0f)?-1.0f : (((val) > 0.0f)?1.0f:0.0f))

#define anki_fMIN(val, min) (((val) < (min))?(min):(val))

// RSSI filtering using an online median estimator.
-(void)readRSSI:(NSNumber*)newRSSI {
#ifdef DEBUG
  if(self.advertisementData) {
    NSData* mfgData = [self.advertisementData objectForKey:CBAdvertisementDataManufacturerDataKey];
    if(mfgData) {
      UInt64 mfgID __attribute__((unused)) = CFSwapInt64BigToHost(*((UInt64*)mfgData.bytes));
      BLELogDebug("BLEAdvertisement.rawVehicleRSSI", "0x%llx %d %lf", mfgID, newRSSI.intValue, self.timeStamp);
    }
  }
  BLELogDebug("BLEAdvertisement.readRSSI", "%d", newRSSI.intValue);
#endif // DEBUG
  
  float diff = newRSSI.floatValue - _rssiMedian;
  _rssiMedian += 1.5f * anki_fsignum(diff);
  _rssiMedian = anki_fMIN(_rssiMedian, -80.f);
}

-(NSNumber*)rssi {
  return [NSNumber numberWithFloat:_rssiMedian];
}

-(void)addServiceID:(CBUUID *)serviceID {
  [_serviceIDs addObject:serviceID];
}

@end
