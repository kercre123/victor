//
//  AnkiSwitchboard.hpp
//  ios-server
//
//  Created by Paul Aluri on 2/5/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef AnkiSwitchboard_hpp
#define AnkiSwitchboard_hpp

#include <stdio.h>
#include <memory>
#include <NetworkExtension/NetworkExtension.h>
#include <CoreBluetooth/CoreBluetooth.h>
#include "simpleSignal.hpp"
#include "libev.h"
#include "SecurePairing.h"
#include "BLEPairingController.h"

#define SB_WIFI_PORT 3291
#define SB_LOOP_TIME 30

namespace Anki {
  class Switchboard {
  public:
    static void Start();
    
    // Pin Updated Event
    using PinUpdatedSignal = Signal::Signal<void (std::string)>;
    static PinUpdatedSignal& OnPinUpdatedEvent() {
      return _PinUpdatedSignal;
    }
    
  private:
    static struct ev_loop* sLoop;
    static Anki::Networking::SecurePairing* securePairing;
    static Signal::SmartHandle pinHandle;
    static Signal::SmartHandle wifiHandle;
    
    static void HandleStartPairing();
    
    static void StartBleComms();
    static void StartWifiComms();
    
    static void OnConnected(Anki::Networking::INetworkStream* stream);
    static void OnDisconnected(Anki::Networking::INetworkStream* stream);
    static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
    
    static PinUpdatedSignal _PinUpdatedSignal;
  };
}

#endif /* AnkiSwitchboard_h */
