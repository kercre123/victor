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

//#include <glib.h>

#include "anki-ble/log.h"
#include "anki-ble/anki_ble_uuids.h"
#include "anki-ble/ble_advertise_settings.h"
#include "switchboardd/_switchboardMain.h"
#include "anki-wifi/wifi.h"

// --------------------------------------------------------------------------------------------------------------------
// Switchboard Daemon
// --------------------------------------------------------------------------------------------------------------------
// @paluri
// --------------------------------------------------------------------------------------------------------------------

void Test();
void OnPinUpdated(std::string pin);
void OnConnected(int connId, Anki::Switchboard::INetworkStream* stream);
void OnDisconnected(int connId, Anki::Switchboard::INetworkStream* stream);

void Anki::Switchboard::Daemon::Start() {
  Log::Write("Loading up Switchboard Daemon");
  _loop = ev_default_loop(0);
  _taskExecutor = std::make_unique<Anki::TaskExecutor>(_loop);

  InitializeBle();
}

void Anki::Switchboard::Daemon::Stop() {
  if(_bleClient != nullptr) {
    _bleClient->Disconnect(_connectionId);
    _bleClient->StopAdvertising();
  }
}

void Anki::Switchboard::Daemon::InitializeBle() {
  _bleClient = std::make_unique<Anki::Switchboard::BleClient>(_loop);
  _bleClient->OnConnectedEvent().SubscribeForever(std::bind(&Daemon::OnConnected, this, std::placeholders::_1, std::placeholders::_2));
  _bleClient->OnDisconnectedEvent().SubscribeForever(std::bind(&Daemon::OnDisconnected, this, std::placeholders::_1, std::placeholders::_2));
  _bleClient->Connect();

  Anki::BLEAdvertiseSettings settings;
  settings.GetAdvertisement().SetServiceUUID(Anki::kAnkiSingleMessageService_128_BIT_UUID);
  settings.GetAdvertisement().SetIncludeDeviceName(true);
  std::vector<uint8_t> mdata = Anki::kAnkiBluetoothSIGCompanyIdentifier;
  mdata.push_back(Anki::kVictorProductIdentifier); // distinguish from future Anki products
  mdata.push_back('p'); // to indicate we are in "pairing" mode
  settings.GetAdvertisement().SetManufacturerData(mdata);
  _bleClient->StartAdvertising(settings);

  Log::Write("Initialize BLE");
}

void Anki::Switchboard::Daemon::OnConnected(int connId, INetworkStream* stream) {
  Log::Write("OnConnected");
  _taskExecutor->Wake([stream, this](){
    Log::Write("Connected to a BLE central.");
    if(_securePairing == nullptr) {
      _securePairing = std::make_unique<Anki::Switchboard::SecurePairing>(stream, _loop);
      _securePairing->OnUpdatedPinEvent().SubscribeForever(std::bind(&Daemon::OnPinUpdated, this, std::placeholders::_1));
    }
    
    // Initiate pairing process
    _securePairing->BeginPairing();
    Log::Write("Done task");
  });
  Log::Write("Done OnConnected");
}

void Anki::Switchboard::Daemon::OnDisconnected(int connId, INetworkStream* stream) {
  // do any clean up needed
  if(_securePairing != nullptr) {
    _securePairing->StopPairing();
  }
}

void Anki::Switchboard::Daemon::OnPinUpdated(std::string pin) {
  Log::Blue((" " + pin + " ").c_str());
}

void Anki::Switchboard::Daemon::Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  // noop
}

// ####################################################################################################################
// Entry Point
// ####################################################################################################################
static struct ev_signal sIntSig;
static struct ev_signal sTermSig;
static ev_timer sTimer;
static struct ev_loop* sLoop;
const static uint32_t kTick_s = 30;

static void ExitHandler(int status = 0) {
  // todo: smoothly handle termination
  _exit(status);
}

static void SignalCallback(struct ev_loop* loop, struct ev_signal* w, int revents)
{
  logi("Exiting for signal %d", w->signum);
  ev_timer_stop(sLoop, &sTimer);
  ev_unloop(sLoop, EVUNLOOP_ALL);
  ExitHandler();
}

static void Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  // noop
}

int main() {
  sLoop = ev_default_loop(0);
  std::unique_ptr<Anki::Switchboard::Daemon> daemon;

  ev_signal_init(&sIntSig, SignalCallback, SIGINT);
  ev_signal_start(sLoop, &sIntSig);
  ev_signal_init(&sTermSig, SignalCallback, SIGTERM);
  ev_signal_start(sLoop, &sTermSig);
  
  // initialize daemon
  daemon = std::make_unique<Anki::Switchboard::Daemon>(sLoop, sTimer);
  daemon->Start();

  // exit
  ev_timer_init(&sTimer, Tick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &sTimer);
  ev_loop(sLoop, 0);
  ExitHandler();
  return 0;
}
// ####################################################################################################################