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
#include "securePairing.h"
#include "BLEPairingController.h"
#include "taskExecutor.h"

namespace Anki {
  class SwitchboardDaemon {
  public:
    static void Start();
    static void Stop();
    
    // Types
    using PinUpdatedSignal = Signal::Signal<void (std::string)>;
    
    // Methods
    static PinUpdatedSignal& OnPinUpdatedEvent() {
      return sPinUpdatedSignal;
    }
    
    static void SetQueue(dispatch_queue_t q) {
      sSwitchboardQueue = q;
    }
    
  private:
    // Methods
    static void HandleStartPairing();
    
    static void StartBleComms();
    static void StopBleComms();
    static void StartWifiComms();
    
    static void TestMessageProtocol(int n);
    
    static void OnConnected(Anki::Switchboard::INetworkStream* stream);
    static void OnDisconnected(Anki::Switchboard::INetworkStream* stream);
    static void OnPinUpdated(std::string pin);
    static void OnReceiveWifiCredentials(std::string ssid, std::string pw);
    static void OnTimerTick(struct ev_loop* loop, struct ev_timer* w, int revents);
    
    // Variables
    const static uint32_t kTick_s = 30;
    
    static struct ev_loop* sLoop;
    static ev_timer sTimer;
    static Anki::Switchboard::SecurePairing* sSecurePairing;
    static dispatch_queue_t sSwitchboardQueue;
    static Anki::TaskExecutor* sTaskExecutor;
    static Signal::SmartHandle sPinHandle;
    static Signal::SmartHandle sWifiHandle;
    
    // todo: tmp connection manager
    static std::unique_ptr<BLEPairingController> sPairingController;
    static PinUpdatedSignal sPinUpdatedSignal;
  };
}

#endif /* AnkiSwitchboard_h */
