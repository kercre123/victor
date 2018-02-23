/**
 * File: _switchboardMain.cpp
 *
 * Author: paluri
 * Created: 2/21/2018
 *
 * Description: Entry point for switchboardd. This program handles
 *              the incoming and outgoing external pairing and
 *              communication between Victor and BLE/WiFi clients.
 *              Switchboard accepts CLAD messages from engine/anim
 *              processes and routes them correctly to attached 
 *              clients, and vice versa. Switchboard also handles
 *              the initial authentication/secure pairing process
 *              which establishes confidential and authenticated
 *              channel of communication between Victor and an
 *              external client.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#include <stdio.h> 
#include <sodium.h>
#include <signals/simpleSignal.hpp>

#include "anki-ble/log.h"
#include "switchboardd/_switchboardMain.h"

// --------------------------------------------------------------------------------------------------------------------
// Switchboard Daemon
// --------------------------------------------------------------------------------------------------------------------
// @paluri
// --------------------------------------------------------------------------------------------------------------------

// Static Variables
struct ev_loop* Anki::Switchboard::Daemon::sLoop;
const uint32_t Anki::Switchboard::Daemon::kTick_s;
ev_timer Anki::Switchboard::Daemon::sTimer;
Anki::TaskExecutor* Anki::Switchboard::Daemon::sTaskExecutor;
int Anki::Switchboard::Daemon::sConnectionId = -1;

Anki::Switchboard::BleClient* Anki::Switchboard::Daemon::sBleClient;
Anki::Switchboard::SecurePairing* Anki::Switchboard::Daemon::sSecurePairing;

void Test();
void OnPinUpdated(std::string pin);
void OnConnected(int connId, Anki::Switchboard::IpcBleStream* stream);
void OnDisconnected(int connId, Anki::Switchboard::IpcBleStream* stream);

void Anki::Switchboard::Daemon::Start() {
  sLoop = ev_default_loop(0);
  sTaskExecutor = new Anki::TaskExecutor(sLoop);

  InitializeBle();

  ev_timer_init(&sTimer, Tick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &sTimer);
  ev_loop(sLoop, 0);
}

void Anki::Switchboard::Daemon::Stop() {
  if(sBleClient != nullptr) {
    sBleClient->Disconnect(sConnectionId);
    sBleClient->StopAdvertising();

    delete sBleClient;
  }

  ev_timer_stop(sLoop, &sTimer);
  ev_unloop(sLoop, EVBREAK_ALL);
}

void Anki::Switchboard::Daemon::InitializeBle() {
  sBleClient = new Anki::Switchboard::BleClient(sLoop);
  sBleClient->OnConnectedEvent().SubscribeForever(OnConnected);
  sBleClient->OnDisconnectedEvent().SubscribeForever(OnDisconnected);
  sBleClient->Connect();
  sBleClient->StartAdvertising();
}

void Anki::Switchboard::Daemon::OnConnected(int connId, INetworkStream* stream) {
  printf("Connected to a BLE central.\n");

  if(sSecurePairing == nullptr) {
    sSecurePairing = new Anki::Switchboard::SecurePairing(stream, sLoop);
    sSecurePairing->OnUpdatedPinEvent().SubscribeForever(OnPinUpdated);
    ///sWifiHandle = sSecurePairing->OnReceivedWifiCredentialsEvent().ScopedSubscribe(OnReceiveWifiCredentials);
  }
  
  // Initiate pairing process
  sSecurePairing->BeginPairing();
}

void Anki::Switchboard::Daemon::OnDisconnected(int connId, INetworkStream* stream) {
  // do any clean up needed
  if(sSecurePairing != nullptr) {
    sSecurePairing->StopPairing();
  }
}

void Anki::Switchboard::Daemon::OnPinUpdated(std::string pin) {
  Log::Write(pin.c_str());
}

void Anki::Switchboard::Daemon::Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  // noop
}

// ####################################################################################################################
// Entry Point
// ####################################################################################################################
int main() {
  Anki::Switchboard::Daemon::Start();
}
// ####################################################################################################################