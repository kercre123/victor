//
//  AnkiSwitchboard.cpp
//
//
//
//  Created by Paul Aluri on 2/5/18.
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#include "switchboard.h"
#include "bleMessageProtocol.h"

struct ev_loop* Anki::SwitchboardDaemon::sLoop;
const uint32_t Anki::SwitchboardDaemon::kTick_s;
ev_timer Anki::SwitchboardDaemon::sTimer;
Anki::SwitchboardDaemon::PinUpdatedSignal Anki::SwitchboardDaemon::sPinUpdatedSignal;
Anki::Switchboard::SecurePairing* Anki::SwitchboardDaemon::sSecurePairing;
Signal::SmartHandle Anki::SwitchboardDaemon::sPinHandle;
Signal::SmartHandle Anki::SwitchboardDaemon::sWifiHandle;
dispatch_queue_t Anki::SwitchboardDaemon::sSwitchboardQueue;
Anki::TaskExecutor* Anki::SwitchboardDaemon::sTaskExecutor;
std::unique_ptr<BLEPairingController> Anki::SwitchboardDaemon::sPairingController;

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
  ev_timer_init(&sTimer, OnTimerTick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &sTimer);
  Log::Write("Initialized . . .");
  ev_loop(sLoop, 0);
  Log::Write(". . . Terminated");
}

void Anki::SwitchboardDaemon::Stop() {
  StopBleComms();
  
  Log::Write("Stopping daemon . . .");
  ev_timer_stop(sLoop, &sTimer);
  ev_unloop(sLoop, EVBREAK_ALL);
}

void Anki::SwitchboardDaemon::StartBleComms() {
  // Tell ankibluetoothd to start advertising.
  sPairingController = std::make_unique<BLEPairingController>();
  
  sPairingController->OnBLEConnectedEvent().SubscribeForever(OnConnected);
  sPairingController->OnBLEDisconnectedEvent().SubscribeForever(OnDisconnected);
  sPairingController->StartAdvertising();
}

void Anki::SwitchboardDaemon::StopBleComms() {
  // Tell ankibluetoothd to stop advertising.
  sPairingController->StopAdvertising();
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

void Anki::SwitchboardDaemon::TestMessageProtocol(int n) {
  // <--
  Anki::Switchboard::BleMessageProtocol* msgProtocol = new Anki::Switchboard::BleMessageProtocol(20);
  
  const int kMsgSize = n;
  
  uint8_t* buffer = (uint8_t*)malloc(kMsgSize);
  for(int i = 0; i < kMsgSize; i++) {
    buffer[i] = i;
  }
  
  msgProtocol->OnReceiveMessageEvent().SubscribeForever([msgProtocol](uint8_t* buffer, size_t size) {
    bool success = true;
    for(int i = 0; i < size; i++) {
      if(buffer[i] != (uint8_t)i) {
        success = false;
        break;
      }
    }
    
    if(!success) {
      printf("Msg protocol test failed!!!\n");
    } else {
      //printf("Msg protocol succeeded for size [%d].\n", size);
    }
  });
  msgProtocol->OnSendRawBufferEvent().SubscribeForever([msgProtocol](uint8_t* buffer, size_t size) {
    msgProtocol->ReceiveRawBuffer(buffer, size);
  });
  
  msgProtocol->SendMessage(buffer, kMsgSize);
  msgProtocol->SendMessage(buffer, kMsgSize);
  
  free(buffer);
  delete msgProtocol;
  // --!>
}
