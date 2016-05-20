//
//  BLECozmoServiceDelegate.h
//  BLEManager
//
//  Created by Brian Chapados on 4/2/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef BLEManager_BLECozmoServiceDelegate_h
#define BLEManager_BLECozmoServiceDelegate_h

@class BLECozmoManager;
@class BLECozmoConnection;

@protocol BLECozmoServiceDelegate <NSObject>
-(void)vehicleManager:(BLECozmoManager*)manager vehicleDidAppear:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisappear:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidConnect:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisconnect:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateAdvertisement:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateProximity:(BLECozmoConnection *)connection;
-(void)vehicleManager:(BLECozmoManager *)manager vehicle:(BLECozmoConnection *)connection didReceiveMessage:(NSData *)message;
@end


#endif
