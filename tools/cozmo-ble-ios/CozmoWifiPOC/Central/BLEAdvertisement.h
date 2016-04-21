//
//  BLEAdvertisement.h
//  BLEManager
//
//  Created by Mark Pauley on 3/29/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>

@class CBPeripheral;
@class CBUUID;

#define RSSI_LOPASS_LENGTH 24 // 500ms smoothing
#define RSSI_MIN_DISCOVER_VALUE -83
#define RSSI_HYSTERISIS_RANGE 18
#define RSSI_MIN_KEEPALIVE_VALUE (RSSI_MIN_DISCOVER_VALUE - RSSI_HYSTERISIS_RANGE)

@interface BLEAdvertisement : NSObject

@property (strong, nonatomic) NSString* name;
@property (strong, nonatomic) CBPeripheral* peripheral;
@property (readonly, nonatomic) NSSet* serviceIDs;
@property (readwrite, nonatomic) CFAbsoluteTime timeStamp;
@property (readonly, nonatomic) NSNumber* rssi;
@property (readwrite, nonatomic) BOOL discovered;
@property (readonly, nonatomic) NSMutableDictionary* advertisementData; // holds the union of all the advertisments

-(void)readRSSI:(NSNumber*)newRSSI;
-(void)addServiceID:(CBUUID*)serviceID;
@end
