
#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

#import "BLECozmoConnection.h"
#import "BLECozmoServiceDelegate.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  // BLE time utility function
uint64_t BLE_TimeInNanoseconds(void);
  
@class BLECentralMultiplexer;
  
/****************************************************************************/
/*							BLECozmoManager class                                         */
/****************************************************************************/
@interface BLECozmoManager : NSObject

@property (strong, nonatomic) NSMutableArray  *vehicleConnections;
@property (strong, nonatomic) NSMutableArray  *pendingPeripheralConnections;
@property (weak, nonatomic)   id<BLECozmoServiceDelegate> serviceDelegate;
@property (strong, nonatomic) dispatch_queue_t queue;


-(id)initWithCentralMultiplexer:(BLECentralMultiplexer*)centralMultiplexer
                          queue:(dispatch_queue_t)queue;

/*
 * Service Discovery
 */

/** Request to receive updates about advertising vehicles
 */
- (void)discoverVehicles;

/** Stop receiving updates about advertising vehicles
 */
- (void)stopDiscoveringVehicles;

/** Determine whether updates about available vehicles will be received.
 *
 * @return YES if we are already scanning for vehicles and will receive advertistment data,
 *         otherwise NO.
 *
 * @note a YES return value only reflects that `discoverVehicles` was called
 * and `stopDiscoveringVehicles` has not been called. This method does not account
 * for whether the underlying state of the BLECentralMultiplexer or BLE subsystem,
 * which may not be scanning (due to pause or other conditions).
 */
- (BOOL)isDiscoveringVehicles;

/*
 * Vehicle Connection
 */

/**
 * Connect to vehicle with mfgId.
 *
 * @return NO if a connection attempt can not start, and no callbacks will be sent.
 *         YES if connect attempt will start and callbacks will be sent.
 */
- (BOOL)connectToVehicleByMfgID:(UInt64)mfgID;

// connection lookup
- (BLECozmoConnection *)connectionForMfgID:(UInt64)mfgID;

/**
 * Disconnect from vehicle with mfgId.
 *
 * @return NO if a disconnect attempt can not start, and no callbacks will be sent.
 *         YES if disconnect attempt will start and callbacks will be sent.
 *
 */
- (BOOL)disconnectVehicleWithMfgId:(UInt64)mfgID
            sendDisconnectMessages:(BOOL)sendDisconnectMessages;

/**
 * Remove all vehicleConnections.
 */
- (void)clearAllVehicles:(BOOL)sendDisconnectMessages;

@end
  
#ifdef __cplusplus
} //extern "C"
#endif
