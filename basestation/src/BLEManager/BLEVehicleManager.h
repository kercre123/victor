/**
 * File: BLEVehicleManager.h
 *
 * Author: Mark Palatucci (markmp)
 * Created: 7/11/2012
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Objective-C class that implements functions called
 * by the CoreBluetooth delegates, and manages the relationships between
 * peripherals, services, characteristics, and vehicleIDs. This will
 * interface with the C++ object that the basestation uses for messaging.
 *
 * Copyright: Anki, Inc. 2008-2012
 *
 **/

#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h> 

#import "BLEManager/Vehicle/BLEVehicleConnection.h"
#import "BLEManager/Vehicle/BLEVehicleMessage.h"


#ifdef __cplusplus
extern "C" {
#endif

// BLE time utility function
uint64_t BLE_TimeInNanoseconds(void);

  
@class BLEVehicleManager;
@protocol BLEVehicleAvailabilityDelegate <NSObject>
-(void)vehicleManager:(BLEVehicleManager*)manager vehicleBecameAvailable:(BLEVehicleConnection*)connection;
-(void)vehicleManager:(BLEVehicleManager*)manager vehicleBecameUnavailable:(BLEVehicleConnection*)connection;
-(void)vehicleManager:(BLEVehicleManager*)manager vehicleStatusChanged:(BLEVehicleConnection*)connection;
@optional
-(void)vehicleManager:(BLEVehicleManager *)manager vehicleRSSIUpdated:(BLEVehicleConnection*)connection;
@end

@protocol BLEVehicleManagerDelegate <NSObject>
-(void)incomingMessagesAvailable;
@end
  
/****************************************************************************/
/*							BLEVehicleManager class                                         */
/****************************************************************************/
@interface BLEVehicleManager : NSObject 

@property (strong, nonatomic) NSMutableArray  *vehicleConnections;
@property (strong, nonatomic) NSMutableArray  *vehicleMessages;
@property (assign) BOOL                       basestationMode;
@property (strong, nonatomic) NSMutableArray  *pendingPeripheralConnections;
@property (weak, nonatomic)   id<BLEVehicleManagerDelegate> delegate;
@property (weak, nonatomic)   id<BLEVehicleAvailabilityDelegate> availabilityDelegate;

+ (BLEVehicleManager*) sharedInstance;

/*
 * Service Discovery
*/
- (void) discoverVehicles;
- (void) stopDiscoveringVehicles;

/*
 * Vehicle Connection
*/

// the 'ID' here is the 1-byte 'vehicleID' typically used by the Anki Drive basestation.
- (void) connectToVehicleByID:(NSInteger)vehicleID __attribute((deprecated));
- (void) connectToVehicleByMfgID:(UInt64)mfgID;

- (void) writeData:(NSData *)data toVehicleByID:(NSInteger)vehicleID;

// A 'polling' read of the connection RSSI.
- (void) readConnectedRSSI:(NSInteger)vehicleID;

// connection lookup
- (BLEVehicleConnection *)connectionForMfgID:(UInt64)mfgID;
- (BLEVehicleConnection *)connectionForVehicleID:(NSInteger)vehicleID;


// FIXME: These should both become deprecated.. the connection should be dropped from the vehicle itself.
- (void) disconnectVehicle:(BLEVehicleConnection *)vehicleConn sendDisconnectMessages:(BOOL)sendDisconnectMessages;
- (void) clearAllVehicles:(BOOL)sendDisconnectMessages;

@end
  
#ifdef __cplusplus
} //extern "C"
#endif
