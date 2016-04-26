//
//  BLECozmoConnection.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif
  
@class CBPeripheral;
@class CBCharacteristic;

@class BLECozmoManager;

// error codes
typedef NS_ENUM(NSInteger, BLECozmoConnectionErrorCode) {
  BLECozmoConnectionErrorCodeNotConnected     = 8000,
  BLECozmoConnectionErrorCodeMessageSizeIncorrect,
};
  
typedef struct {
  NSUInteger _batchLength;
  NSUInteger _batchDelayMillisec;
} BLECozmoonnectionThrottlingParams;
  
// Max number of messages to send in one batch
constexpr int kBLECozmoConnectionMessageBatchLength = 2;
  
// Delay (ms) between batched message writes
constexpr int kBLECozmoConnectionMessageBatchDelay_MSEC = 30;
  
enum _BLEConnectionState {
  kUnavailable           = -1,
  kDisconnected          = 0,
  kPending               = 1,
  kConnecting            = 2,
  kConnectedPipeNotReady = 3,
  kConnectedPipeReady    = 4
};
  
typedef int BLEConnectionState;
  
@interface BLECozmoConnection : NSObject

// back pointer to parent manager
@property (weak, nonatomic) BLECozmoManager*      cozmoManager;

@property (assign, nonatomic)  NSInteger          scannedRSSI;
@property (assign, nonatomic) BLEConnectionState  connectionState;

// TODO: We might want to think about storing these as Identifiers/UUIDs
// and letting BLECentralMultiplexer own the actual objects.
// That would avoid any potential issues related to cached peripheral or characteristic state.
@property (strong, nonatomic) CBPeripheral*       carPeripheral;
@property (strong, nonatomic) CBCharacteristic*   toCarCharacteristic;
@property (strong, nonatomic) CBCharacteristic*   toPhoneCharacteristic;

// advertised data
@property (strong, nonatomic) NSData*             mfgData;
@property (readonly, nonatomic) NSDictionary*     advertisementData;
@property (readonly, nonatomic) UInt64            mfgID;

@property (assign, nonatomic) BOOL canWrite;
@property (strong, nonatomic) NSMutableArray*     enqueuedMessages;
@property (assign, nonatomic, getter=isThrottlingDisabled) BOOL throttlingDisabled;
@property (assign, nonatomic) NSUInteger          messageBatchLength;
@property (assign, nonatomic) NSUInteger          messageBatchDelayMilliSec;

@property (assign, nonatomic) UInt32              messagesSent;
@property (assign, nonatomic) UInt32              bytesSent;
@property (assign, nonatomic) UInt32              messagesReceived;
@property (assign, nonatomic) UInt32              bytesReceived;

// Max amount of time to wait between firmware packet writes
// before the operation times out.
// Default value = 3.0 seconds
@property (nonatomic, assign) NSTimeInterval latencyTimeout;

@property (assign, nonatomic) dispatch_queue_t throttlingQueue;

// designated initializer
- (id)initWithPeripheral:(CBPeripheral *)peripheral vehicleManager:(BLECozmoManager *)manager;

- (void)updateWithAdvertisementData:(NSDictionary*)newAdvertisementData;
- (void)clearAdvertisementState;

- (void)clearConnectionState;

//- (BOOL)sendUpdateWithName:(NSString *)name error:(NSError *__autoreleasing *)error;

// used to query RSSI data when a peripheral is connected.
- (void)readConnectedRSSI;

// Buffered I/O
// Send data to carPeripheral, using a throttling mechanism to avoid
// saturating the connection.
- (BOOL)writeMessageData:(NSData *)data error:(NSError *__autoreleasing *)error encrypted:(BOOL)encrypted;

// Unbuffered I/O
// Write data directly to the carPeripheral.
- (void)disconnectFromPeripheral;

@end
  
#ifdef __cplusplus
} //extern "C"
#endif
