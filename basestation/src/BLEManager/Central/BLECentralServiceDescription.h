//
//  BLECentralServiceDescription.h
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h>

// Simple protocol for delegation of events from a two-way BTLE service.
@class BLECentralServiceDescription;

@protocol BLECentralServiceDelegate <NSObject>
-(void)service:(BLECentralServiceDescription*)service didDiscoverPeripheral:(CBPeripheral*)peripheral withRSSI:(NSNumber*)rssiVal advertisementData:(NSDictionary*)advertisementData;
-(void)service:(BLECentralServiceDescription*)service peripheralDidDisappear:(CBPeripheral*)peripheral;
-(void)service:(BLECentralServiceDescription*)service didDisconnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error;
-(void)service:(BLECentralServiceDescription*)service discoveredCharacteristic:(CBCharacteristic*)characteristic forPeripheral:(CBPeripheral*)peripheral;
-(void)service:(BLECentralServiceDescription*)service didUpdateNotificationStateForCharacteristic:(CBCharacteristic*)characteristic forPeripheral:(CBPeripheral*)peripheral error:(NSError*)error;
-(void)service:(BLECentralServiceDescription*)service incomingMessage:(NSData*)message onCharacteristic:(CBCharacteristic*)characteristic forPeripheral:(CBPeripheral*)peripheral;
-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didUpdateRSSI:(NSNumber*)rssiVal;
-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didUpdateAdvertisementData:(NSDictionary*)advertisementData;
-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
         error:(NSError *)error;
@end

@interface BLECentralServiceDescription : NSObject

// service UUID
@property (nonatomic,strong) CBUUID* serviceID;

// Array of characteristic UUIDs
@property (nonatomic,strong) NSArray* characteristicIDs;

// service delegate
@property (nonatomic,weak) id<BLECentralServiceDelegate> delegate;

// service name
@property (nonatomic,strong) NSString* name;

@end
