//
//  AnkiSwitchboard.cpp
//
//
//
//  Created by Paul Aluri on 2/5/18.
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#include "switchboard.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DEBUG DEFINE FOR TESTING ON iOS !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define IOS_MAIN 1
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct ev_loop* Anki::Switchboard::sLoop;
Anki::Switchboard::PinUpdatedSignal Anki::Switchboard::_PinUpdatedSignal;
Anki::Networking::SecurePairing* Anki::Switchboard::securePairing;
Signal::SmartHandle Anki::Switchboard::pinHandle;
Signal::SmartHandle Anki::Switchboard::wifiHandle;
dispatch_queue_t Anki::Switchboard::switchboardQueue;
Anki::TaskExecutor* Anki::Switchboard::_sTaskExecutor;

void Anki::Switchboard::Start() {
  // Set static loop
  sLoop = ev_default_loop(0);
  _sTaskExecutor = new Anki::TaskExecutor(sLoop);
  ////////////////////////////////////////
  // Setup BLE process comms
  ////////////////////////////////////////
  //
  // 1. Hookup watchers for ankibluetoothd domain sockets.
  StartBleComms();
  //
  // ###

  ////////////////////////////////////////
  // Setup WiFi process comms
  ////////////////////////////////////////
  //
  // 1. Hookup watchers for connman.
  // 2. Create socket for WiFi communication.
  //
  // ###

  ////////////////////////////////////////
  // Setup secure pairing handler
  ////////////////////////////////////////
  //
  // 1. Instantiate SecurePairing object? Or do we instantiate it ad hoc...probably ad hoc?
  // 2. Hook into watcher to know when to begin pairing (this will probably be domain socket?)
  //
  // ###
  
  ////////////////////////////////////////
  // Setup sdk clad messaging handler
  ////////////////////////////////////////
  //
  // @Shawn Blakesley
  //
  // ###
  
  ////////////////////////////////////////
  // Setup Network Interface
  ////////////////////////////////////////
  //
  // 1. Create IPC for WiFi and BLE Send/Receive
  //    -- These should only expose encrypted channel.
  //
  // ###

  // Begin event loop
  ev_timer timer;
  ev_timer_init(&timer, sEvTimerHandler, SB_LOOP_TIME, SB_LOOP_TIME);
  ev_timer_start(sLoop, &timer);
  Log::Write("Initialized . . .");
  ev_loop(sLoop, 0);
  Log::Write(". . . Terminated");
}

void Anki::Switchboard::sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents)
{
  // Log::Write("[timer] Outer tick.");
}

void Anki::Switchboard::StartBleComms() {
  // Tell ankibluetoothd to start advertising.
  BLEPairingController* pairingController = new BLEPairingController();
  
  pairingController->OnBLEConnectedEvent().SubscribeForever(OnConnected);
  pairingController->OnBLEDisconnectedEvent().SubscribeForever(OnDisconnected);
  pairingController->StartAdvertising();
}

void Anki::Switchboard::StartWifiComms() {
  // Start WiFi server
  // ##
}

void Anki::Switchboard::HandleStartPairing() {
  
}

void Anki::Switchboard::OnDisconnected(Anki::Networking::INetworkStream* stream) {
  _sTaskExecutor->Wake([stream]() {
    Log::Write("Disconnected from BLE central");
    securePairing->StopPairing();
  });
}

void Anki::Switchboard::OnPinUpdated(std::string pin) {
  _PinUpdatedSignal.emit(pin);
}

void Anki::Switchboard::OnReceiveWifiCredentials(std::string ssid, std::string pw) {
  NEHotspotConfiguration* wifi = [[NEHotspotConfiguration alloc] initWithSSID:[NSString stringWithUTF8String:ssid.c_str()] passphrase:[NSString stringWithUTF8String:pw.c_str()] isWEP:false];
  NSLog(@"Connecting to : [%s], [%s]", ssid.c_str(), pw.c_str());
  [[NEHotspotConfigurationManager sharedManager] applyConfiguration:wifi completionHandler:nullptr];
}

void Anki::Switchboard::OnConnected(Anki::Networking::INetworkStream* stream) {
  _sTaskExecutor->Wake([stream]() {
    Log::Write("Connected to a BLE central.");
    
    if(securePairing == nullptr) {
      securePairing = new Anki::Networking::SecurePairing(stream, ev_default_loop(0));
      pinHandle = securePairing->OnUpdatedPinEvent().ScopedSubscribe(OnPinUpdated);
      wifiHandle = securePairing->OnReceivedWifiCredentialsEvent().ScopedSubscribe(OnReceiveWifiCredentials);
    }
    
    // Initiate pairing process
    securePairing->BeginPairing();
  });
}

# if !IOS_MAIN
int main() {
  Anki::Switchboard::Start();
}
# endif
