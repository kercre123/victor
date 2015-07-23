//
//  BLEVehicleConnection.h
//  BLEManager
//
//  Created by Mark Pauley on 4/8/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BLEManager/Vehicle/BLEVehicleMessage.h"

#ifdef __cplusplus
extern "C" {
#endif

@class CBPeripheral;
@class CBCharacteristic;

// error codes
typedef NS_ENUM(NSInteger, BLEVehicleConnectionErrorCode) {
  BLEVehicleConnectionErrorCodeNotConnected     = 8000,
  BLEVehicleConnectionErrorCodeNoFirmware,
  BLEVehicleConnectionErrorCodeMessageTooLong,
  BLEVehicleConnectionErrorCodeGameFlashInvalid
};
  
// BLE Advertising packet data format.
  
struct _advertised_vehicle_state {
  uint8_t _reservedState_0:1; // 0: unused
  uint8_t _reservedState_1:1; // 1: unused
  uint8_t _reservedState_2:1; // 2: unused
  uint8_t onTrack:1;          // 3: (reserved - TRUE for onTrack detection)
  uint8_t fullBattery:1;      // 4: TRUE if Car has full battery
  uint8_t lowBattery:1;       // 5: TRUE if Car has low battery
  uint8_t onCharger:1;        // 6: TRUE if Car is on Charger
  uint8_t _notAvailable:1;    // 7: Cannot be used due to BTLE character set constraint
};
  
typedef struct {
  // Vehicle State
  union {
    uint8_t value;
    struct _advertised_vehicle_state flags;
  } state;
  uint16_t firmwareVersion;     // firmware version
  uint8_t _reservedVehicle;     // reserved for vehicle state expansion
  uint8_t _reservedFlash[2];    // reserved for game flash expansion
  uint16_t upgrades;            // contains the upgrades the user has put on the car
  uint8_t name[13];             // nickname for the car. 12 bytes max + null terminator
} BLEVehicleAdvertisedPacket_V0;

typedef struct {
  // Vehicle State
  union {
    uint8_t value;
    struct _advertised_vehicle_state flags;
  } state;
  uint16_t firmwareVersion;     // firmware version
  uint8_t _reservedVehicle;     // reserved for vehicle state expansion
  uint32_t upgrades;            // contains the upgrades the user has put on the car
  uint8_t name[13];             // nickname for the car. 12 bytes max + null terminator
} BLEVehicleAdvertisedPacket_V1;

typedef BLEVehicleAdvertisedPacket_V1 BLEVehicleAdvertisedPacket;

extern BOOL BLEVehicleAdvertisedPacketEqualToPacket(BLEVehicleAdvertisedPacket a, BLEVehicleAdvertisedPacket b);


enum _BLEConnectionState {
  kUnavailable           = -1,
  kDisconnected          = 0,
  kPending               = 1,
  kConnecting            = 2,
  kConnectedPipeNotReady = 3,
  kConnectedPipeReady    = 4
};

typedef int BLEConnectionState;
  

// Max number of messages to send in one batch
#define BLEVehicleConnectionMessageBatchLength 3

// Delay (ms) between batched message writes
#define BLEVehicleConnectionMessageBatchDelay_MSEC 40

@interface BLEVehicleConnection : NSObject

@property (assign, nonatomic)  NSInteger              vehicleID;  // really an unsigned char: can't be over 255
@property (assign, nonatomic)  NSInteger                  scannedRSSI;
@property (assign, nonatomic) BLEConnectionState         connectionState;
@property (strong, nonatomic) CBPeripheral     *carPeripheral;
@property (strong, nonatomic) CBCharacteristic *toCarCharacteristic;
@property (strong, nonatomic) CBCharacteristic *toPhoneCharacteristic;

// advertised data
@property (strong, nonatomic) NSData           *mfgData;
@property (readonly, nonatomic) NSDictionary   *advertisementData;
@property (readonly, nonatomic) UInt64         mfgID;
@property (readonly, nonatomic) BLEVehicleAdvertisedPacket advertisedPacket;
@property (readonly, nonatomic) NSString       *name;
@property (readonly, nonatomic) unsigned int   upgrades;
@property (readonly, nonatomic) unsigned char  stateByte;
@property (readonly, nonatomic) BOOL           isOnTrack;
@property (readonly, nonatomic) BOOL           isBatteryFull;
@property (readonly, nonatomic) BOOL           isBatteryLow;
@property (readonly, nonatomic) BOOL           isOnCharger;
@property (readonly, nonatomic) uint16_t       buildVersion;
@property (readonly, nonatomic) BOOL           needsFirmware;

@property (assign, nonatomic) BOOL canWrite;
@property (strong, nonatomic) NSMutableArray *enqueuedMessages;
@property (assign, nonatomic, getter=isThrottlingDisabled) BOOL throttlingDisabled;

@property (assign, nonatomic) UInt32 messagesSent;
@property (assign, nonatomic) UInt32 bytesSent;
@property (assign, nonatomic) UInt32 messagesReceived;
@property (assign, nonatomic) UInt32 bytesReceived;

@property (assign, nonatomic) dispatch_queue_t throttlingQueue;

// designated initializer
- (id)initWithPeripheral:(CBPeripheral *)peripheral;

- (void)updateWithAdvertisementData:(NSDictionary*)newAdvertisementData;
- (void)clearAdvertisementState;

//- (BOOL)sendUpdateWithName:(NSString *)name error:(NSError *__autoreleasing *)error;

// used to query RSSI data when a peripheral is connected.
- (void)readConnectedRSSI;

// Buffered I/O
// Send data to carPeripheral, using a throttling mechanism to avoid
// saturating the connection.
- (BOOL)writeMessageData:(NSData *)data error:(NSError *__autoreleasing *)error;
- (BOOL)writeData:(NSData *)data error:(NSError *__autoreleasing *)error;
- (BOOL)disconnect:(NSError *__autoreleasing *)error;

// Unbuffered I/O
// Write data directly to the carPeripheral.
- (void)writeDataToPeripheral:(NSData *)data;
- (void)disconnectFromPeripheral;

// Input responders (get the first crack at the given messages for this vehicle connection)
//  The selector in question will need to take two arguments, a BLEVehicleMessage and a BLEVehicleConnection.
//  -(void)handleMessage:(BLEVehicleMessage*)messageType onConnection:(BLEVehicleConnection*)connection;
//  Responders should return YES to indicate that the message was handled and the it should not be
//  forwarded or queued.  Responders should return NO to indicate that the message can be passed along
//  the responder chain.
-(void)setResponder:(id<NSObject>)responder selector:(SEL)selector forMessageType:(BLEVehicleMessageType)messageType;
-(void)removeResponder:(id<NSObject>)responder forMessageType:(BLEVehicleMessageType)type;
-(void)removeResponderForAllMessageTypes:(id<NSObject>)responder;

// private, internal use only
// Send message to a responder object.
// @return YES if this connection instance sent the message, or NO if no responder was found.
// @parameter message The message to send
// @parameter handled A BOOL containing the responders return value.
-(BOOL)sendMessageToResponder:(BLEVehicleMessage*)message handled:(BOOL *)handled;

@end
  
#ifdef __cplusplus
} //extern "C"
#endif
