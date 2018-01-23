//
//  BLEPairingController.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BLEPairingController_h
#define BLEPairingController_h

#include "BLEPeripheral.h"
#include "BLENetworkStream.h"

class BLEPairingController {
public:
  void StartAdvertising() {
    _BLEPeripheral = [[BLEPeripheral alloc] init];
    [_BLEPeripheral setConnectedSignal:&_BLEConnectedSignal];
    [_BLEPeripheral advertiseWithService:LOCAL_NAME withService:PAIRING_SERVICE];
  };
  
  using BLEConnectedSignal = Signal::Signal<void (Anki::Networking::BLENetworkStream*)>;
  
  // Message Receive Event
  BLEConnectedSignal& OnBLEConnectedEvent() {
    return _BLEConnectedSignal;
  }
  
private:
  const char* PAIRING_SERVICE = "00112233-4455-6677-8899-887766554433";
  const char* LOCAL_NAME = "Victor D0G3"; // TODO: This needs to be random generated, and not permanent
  
  BLEPeripheral* _BLEPeripheral;
  BLEConnectedSignal _BLEConnectedSignal;
};

#endif /* BLEPairingController_h */
