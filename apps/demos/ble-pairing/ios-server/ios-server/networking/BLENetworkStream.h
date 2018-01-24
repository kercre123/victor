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
  namespace Networking {
    class BLENetworkStream : public INetworkStream {
    public:
      BLENetworkStream(CBPeripheralManager* manager, CBMutableCharacteristic* writeCharacteristic, CBMutableCharacteristic* encryptedWriteCharacteristic) {
        _peripheralManager = manager;
        _writeCharacteristic = writeCharacteristic;
        _encryptedWriteCharacteristic = encryptedWriteCharacteristic;
      }
      
      void SendPlainText(uint8_t* bytes, int length);
      void SendEncrypted(uint8_t* bytes, int length);
      
    private:
      CBPeripheralManager* _peripheralManager;
      CBMutableCharacteristic* _writeCharacteristic;
      CBMutableCharacteristic* _encryptedWriteCharacteristic;
    };
  }
}

#endif /* BLENetworkStream_h */
