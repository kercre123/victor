//
//  AnkiSwitchboard.cpp
//
//
//
//  Created by Paul Aluri on 2/5/18.
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#include "Switchboard.h"
#include "SecurePairing.h"

#define IOS_MAIN

struct ev_loop* Anki::Switchboard::sLoop;

void Anki::Switchboard::Start() {
  // Set static loop
  sLoop = ev_default_loop(0);

  ////////////////////////////////////////
  // Setup BLE process comms
  ////////////////////////////////////////
  //
  // 1. Hookup watchers for ankibluetoothd domain sockets.
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
  ev_loop(sLoop, 0);
}

void Anki::Switchboard::StartBleComms() {
  // Tell ankibluetoothd to start advertising.
  
}

void Anki::Switchboard::StartWifiComms() {
  // Start WiFi server
  // ##
}

void Anki::Switchboard::HandleStartPairing() {
  //
  // if(_BleConnection->IsConnected()) {
  //   _SecurePairing = std::make_unique<Networking::SecurePairing>(_BleConnection->GetStream(), sLoop);
  // }
}

# ifndef IOS_MAIN
int main() {
  Anki::Switchboard::Start();
}
# endif
