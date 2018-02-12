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

struct ev_loop* Anki::SwitchboardDaemon::sLoop;
Anki::SwitchboardDaemon::PinUpdatedSignal Anki::SwitchboardDaemon::sPinUpdatedSignal;
Anki::Switchboard::SecurePairing* Anki::SwitchboardDaemon::sSecurePairing;
Signal::SmartHandle Anki::SwitchboardDaemon::sPinHandle;
Signal::SmartHandle Anki::SwitchboardDaemon::sWifiHandle;
dispatch_queue_t Anki::SwitchboardDaemon::sSwitchboardQueue;
Anki::TaskExecutor* Anki::SwitchboardDaemon::sTaskExecutor;
BLEPairingController* Anki::SwitchboardDaemon::sPairingController;
const uint32_t Anki::SwitchboardDaemon::kTick_s;

void Anki::SwitchboardDaemon::Start() {
  // Set static loop
  sLoop = ev_default_loop(0);
  sTaskExecutor = new Anki::TaskExecutor(sLoop);
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
  ev_timer_init(&timer, OnTimerTick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &timer);
  Log::Write("Initialized . . .");
  ev_loop(sLoop, 0);
  Log::Write(". . . Terminated");
}

void Anki::SwitchboardDaemon::StartBleComms() {
  // Tell ankibluetoothd to start advertising.
  sPairingController = new BLEPairingController();
  
  sPairingController->OnBLEConnectedEvent().SubscribeForever(OnConnected);
  sPairingController->OnBLEDisconnectedEvent().SubscribeForever(OnDisconnected);
  sPairingController->StartAdvertising();
}

void Anki::SwitchboardDaemon::StopBleComms() {
  // Tell ankibluetoothd to stop advertising.
  sPairingController->StopAdvertising();
  
  delete sPairingController;
}

void Anki::SwitchboardDaemon::StartWifiComms() {
  // Start WiFi server
  // ##
}

void Anki::SwitchboardDaemon::HandleStartPairing() {
  
}

void Anki::SwitchboardDaemon::OnDisconnected(Anki::Switchboard::INetworkStream* stream) {
  sTaskExecutor->Wake([stream]() {
    Log::Write("Disconnected from BLE central");
    sSecurePairing->StopPairing();
  });
}

void Anki::SwitchboardDaemon::OnPinUpdated(std::string pin) {
  sPinUpdatedSignal.emit(pin);
}

void Anki::SwitchboardDaemon::OnReceiveWifiCredentials(std::string ssid, std::string pw) {
  NEHotspotConfiguration* wifi = [[NEHotspotConfiguration alloc] initWithSSID:[NSString stringWithUTF8String:ssid.c_str()] passphrase:[NSString stringWithUTF8String:pw.c_str()] isWEP:false];
  NSLog(@"Connecting to : [%s], [%s]", ssid.c_str(), pw.c_str());
  [[NEHotspotConfigurationManager sharedManager] applyConfiguration:wifi completionHandler:nullptr];
}

void Anki::SwitchboardDaemon::OnConnected(Anki::Switchboard::INetworkStream* stream) {
  sTaskExecutor->Wake([stream]() {
    Log::Write("Connected to a BLE central.");
    
    if(sSecurePairing == nullptr) {
      sSecurePairing = new Anki::Switchboard::SecurePairing(stream, ev_default_loop(0));
      sPinHandle = sSecurePairing->OnUpdatedPinEvent().ScopedSubscribe(OnPinUpdated);
      sWifiHandle = sSecurePairing->OnReceivedWifiCredentialsEvent().ScopedSubscribe(OnReceiveWifiCredentials);
    }
    
    // Initiate pairing process
    sSecurePairing->BeginPairing();
  });
}

void Anki::SwitchboardDaemon::OnTimerTick(struct ev_loop* loop, struct ev_timer* w, int revents)
{
  // no op
}

# if !IOS_MAIN
int main() {
  Anki::SwitchboardDaemon::Start();
}
# endif
