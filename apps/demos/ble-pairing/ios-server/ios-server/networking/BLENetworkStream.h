//
//  BLENetworkStream.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BLENetworkStream_h
#define BLENetworkStream_h

#include <CoreBluetooth/CoreBluetooth.h>
#include "INetworkStream.h"

namespace Anki {
namespace Switchboard {
  class BLENetworkStream : public INetworkStream {
  public:
    BLENetworkStream(CBPeripheralManager* manager, CBMutableCharacteristic* writeCharacteristic, CBMutableCharacteristic* encryptedWriteCharacteristic) {
      _peripheralManager = manager;
      _writeCharacteristic = writeCharacteristic;
      _encryptedWriteCharacteristic = encryptedWriteCharacteristic;
    }
    
    int SendPlainText(uint8_t* bytes, int length) __attribute__((warn_unused_result));
    int SendEncrypted(uint8_t* bytes, int length) __attribute__((warn_unused_result));
    
  private:
    CBPeripheralManager* _peripheralManager;
    CBMutableCharacteristic* _writeCharacteristic;
    CBMutableCharacteristic* _encryptedWriteCharacteristic;
  };
} // Switchboard
} // Anki

#endif /* BLENetworkStream_h */
