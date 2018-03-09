//
//  BleClient.h
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BleClient_h
#define BleClient_h

#include <CoreBluetooth/CoreBluetooth.h>
#include <stdlib.h>
#include "bleMessageProtocol.h"
#include "messageExternalComms.h"
#include "sodium/include/sodium.h"

enum RtsState {
  Raw         = 0,
  Clad        = 1,
  CladSecure  = 2,
};

@interface BleCentral : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
  NSString* _localName;
  
  CBCentralManager* _centralManager;
  CBUUID* _victorService;
  CBUUID* _readUuid;
  CBUUID* _writeUuid;
  CBUUID* _readSecureUuid;
  CBUUID* _writeSecureUuid;
  
  CBPeripheral* _peripheral;
  
  NSMutableDictionary* _characteristics;
  
  std::unique_ptr<Anki::Switchboard::BleMessageProtocol> _bleMessageProtocol;
  
  uint8_t _publicKey [crypto_kx_PUBLICKEYBYTES];
  uint8_t _secretKey [crypto_kx_SECRETKEYBYTES];
  uint8_t _encryptKey [crypto_kx_SESSIONKEYBYTES];
  uint8_t _decryptKey [crypto_kx_SESSIONKEYBYTES];
  uint8_t _remotePublicKey [crypto_kx_PUBLICKEYBYTES];
  uint8_t _nonceIn [crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
  uint8_t _nonceOut [crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
  
  enum RtsState _rtsState;
  
  bool _victorIsPairing;
  bool _connecting;
}

- (void) handleSend:(const void*)bytes length:(int)n;
- (void) handleReceive:(const void*)bytes length:(int)n;
- (void) handleReceiveSecure:(const void*)bytes length:(int)n;

- (void) HandleReceiveHandshake:(const void*)bytes length:(int)n;
- (void) HandleReceivePublicKey:(const Anki::Victor::ExternalComms::RtsConnRequest&)msg;
- (void) HandleReceiveNonce:(const Anki::Victor::ExternalComms::RtsNonceMessage&)msg;
- (void) HandleChallengeMessage:(const Anki::Victor::ExternalComms::RtsChallengeMessage&)msg;
- (void) HandleChallengeSuccessMessage:(const Anki::Victor::ExternalComms::RtsChallengeSuccessMessage&)msg;
- (void) HandleWifiScanResponse:(const Anki::Victor::ExternalComms::RtsWifiScanResponse&)msg;
- (void) HandleWifiConnectResponse:(const Anki::Victor::ExternalComms::RtsWifiConnectResponse&)msg;
- (void) HandleReceiveAccessPointResponse:(const Anki::Victor::ExternalComms::RtsWifiAccessPointResponse&)msg;

- (void) send:(const void*)bytes length:(int)n;
- (void) sendSecure:(const void*)bytes length:(int)n;

- (void) StartScanning;
- (void) StopScanning;

- (void) printSuccess:(const char*) txt;

// for reconnection
- (bool) HasSavedPublicKey;
- (bool) HasSavedSession: (NSString*)key;
- (NSData*) GetPublicKey;
- (NSArray*) GetSession: (NSString*)key;
- (void)resetDefaults;

@end

class Clad {
public:
  template<typename T, typename... Args>
  static void SendRtsMessage(BleCentral* central, Args&&... args) {
    Anki::Victor::ExternalComms::ExternalComms msg = Anki::Victor::ExternalComms::ExternalComms(Anki::Victor::ExternalComms::RtsConnection(T(std::forward<Args>(args)...)));
    std::vector<uint8_t> messageData(msg.Size());
    const size_t packedSize = msg.Pack(messageData.data(), msg.Size());
    [central send:messageData.data() length:packedSize];
  }
};

#endif /* BleClient_h */
