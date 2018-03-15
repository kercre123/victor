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
#include <unistd.h>
#include <stdio.h> 
#include <sodium.h>
#include <signals/simpleSignal.hpp>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <fstream>

#include "anki-ble/log.h"
#include "anki-ble/anki_ble_uuids.h"
#include "anki-ble/ble_advertise_settings.h"
#include "anki-wifi/wifi.h"
#include "switchboardd/main.h"

// --------------------------------------------------------------------------------------------------------------------
// Switchboard Daemon
// --------------------------------------------------------------------------------------------------------------------
// @paluri
// --------------------------------------------------------------------------------------------------------------------

namespace Anki {
namespace Switchboard {

void Daemon::Start() {
  Log::Write("Loading up Switchboard Daemon");
  _loop = ev_default_loop(0);
  _taskExecutor = std::make_unique<Anki::TaskExecutor>(_loop);

  InitializeEngineComms();
  Log::Write("Finished Starting");

  // Initialize Ota Timer
  _handleOtaTimer.signal = &_otaUpdateTimerSignal;
  _otaUpdateTimerSignal.SubscribeForever(std::bind(&Daemon::HandleOtaUpdateProgress, this));
  ev_timer_init(&_handleOtaTimer.timer, &Daemon::sEvTimerHandler, kOtaUpdateInterval_s, kOtaUpdateInterval_s);
}

void Daemon::Stop() {
  if(_bleClient != nullptr) {
    _bleClient->Disconnect(_connectionId);
    _bleClient->StopAdvertising();
  }

  if(_engineMessagingClient != nullptr) {
    Log::Write("End pairing state.");
    _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
  }

  ev_timer_stop(_loop, &_handleOtaTimer.timer);
}

void Daemon::InitializeEngineComms() {
  _engineMessagingClient = std::make_shared<EngineMessagingClient>(_loop);
  _engineMessagingClient->Init();
  _engineMessagingClient->OnReceivePairingStatus().SubscribeForever(std::bind(&Daemon::OnPairingStatus, this, std::placeholders::_1));
  _engineTimer.daemon = this;
  ev_timer_init(&_engineTimer.timer, HandleEngineTimer, 0.1f, 0.1f);
  ev_timer_start(_loop, &_engineTimer.timer);
}

bool Daemon::TryConnectToEngineServer() {
  bool connected = _engineMessagingClient->Connect();

  if (connected) {
    Log::Write("Initialize EngineMessagingClient");
  } else {
    Log::Write("Failed to Initialize EngineMessagingClient ... trying again.");
  }

  return connected;
}

void Daemon::InitializeBleComms() {
  Log::Write("Initialize BLE");
  _bleClient = std::make_unique<Anki::Switchboard::BleClient>(_loop);
  _bleClient->OnConnectedEvent().SubscribeForever(std::bind(&Daemon::OnConnected, this, std::placeholders::_1, std::placeholders::_2));
  _bleClient->OnDisconnectedEvent().SubscribeForever(std::bind(&Daemon::OnDisconnected, this, std::placeholders::_1, std::placeholders::_2));

  bool connected = _bleClient->Connect();

  if(connected) {
    Log::Write("Ble IPC client connected.");
    UpdateAdvertisement(false);
  } else {
    Log::Write("Fatal error. Could not connect to ankibluetoothd.");
    // todo:// should probably exit program so that systemd will restart?
  }
}

void Daemon::UpdateAdvertisement(bool pairing) {
  if(_bleClient == nullptr || !_bleClient->IsConnected()) {
    Log::Write("Tried to update BLE advertisement when not connected to ankibluetoothd.");
    return;
  }

  // update state
  _isPairing = pairing;

  if(_securePairing != nullptr) {
    _securePairing->SetIsPairing(pairing);
  }

  Anki::BLEAdvertiseSettings settings;
  std::vector<uint8_t> mdata;
  settings.GetAdvertisement().SetServiceUUID(Anki::kAnkiSingleMessageService_128_BIT_UUID);
  settings.GetAdvertisement().SetIncludeDeviceName(true);
  mdata = Anki::kAnkiBluetoothSIGCompanyIdentifier;
  mdata.push_back(Anki::kVictorProductIdentifier); // distinguish from future Anki products
  mdata.push_back(pairing?'p':0x00); // to indicate whether we are pairing
  settings.GetAdvertisement().SetManufacturerData(mdata);
  _bleClient->StartAdvertising(settings);
}

void Daemon::OnConnected(int connId, INetworkStream* stream) {
  Log::Write("OnConnected");
  _taskExecutor->Wake([stream, this](){
    Log::Write("Connected to a BLE central.");
    if(_securePairing == nullptr) {
      _securePairing = std::make_unique<Anki::Switchboard::SecurePairing>(stream, _loop, _engineMessagingClient, _isPairing, _isOtaUpdating);
      _securePairing->OnUpdatedPinEvent().SubscribeForever(std::bind(&Daemon::OnPinUpdated, this, std::placeholders::_1));
      _securePairing->OnOtaUpdateRequestEvent().SubscribeForever(std::bind(&Daemon::OnOtaUpdatedRequest, this, std::placeholders::_1));
    }
    
    // Initiate pairing process
    _securePairing->BeginPairing();
    Log::Write("Done task");
  });
  Log::Write("Done OnConnected");
}

void Daemon::OnDisconnected(int connId, INetworkStream* stream) {
  // do any clean up needed
  if(_securePairing != nullptr) {
    _securePairing->StopPairing();
    Log::Write("BLE Central disconnected.");
    UpdateAdvertisement(false);
    _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
    Log::Write("Destroying secure pairing object.");
    _securePairing = nullptr;
  }
}

void Daemon::OnPinUpdated(std::string pin) {
  _engineMessagingClient->SetPairingPin(pin);
  _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::START_PAIRING);
  _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::SHOW_PIN);
  Log::Blue((" " + pin + " ").c_str());
}

void Daemon::HandleOtaUpdateProgress() {
  if(_securePairing != nullptr) {
    // Update connected client of status
    int progressVal = 0;
    int expectedVal = 0;

    int status = GetOtaProgress(&progressVal, &expectedVal);

    if(status == -1) {
      _securePairing->SendOtaProgress(254, progressVal, expectedVal);
      return;  
    }

    Log::Write("Downloaded %d/%d bytes.", progressVal, expectedVal);
    _securePairing->SendOtaProgress(0, progressVal, expectedVal);
  }
}

int Daemon::GetOtaProgress(int* progressVal, int* expectedVal) {
  // read values from files
  std::string progress;
  std::string expected;

  std::ifstream progressFile;
  std::ifstream expectedFile;

  progressFile.open("/data/update-engine/progress");
  expectedFile.open("/data/update-engine/expected-size");

  if(!progressFile.is_open() || !expectedFile.is_open()) {
    return -1;
  }

  getline(progressFile, progress);
  getline(expectedFile, expected);

  *progressVal = stoi(progress);
  *expectedVal = stoi(expected);

  return 0;
}

void Daemon::HandleOtaUpdateExit(int rc, const std::string& output) {
  if(rc == 0) {
    // todo: inform client of status before rebooting
    if(_securePairing != nullptr) {
      _securePairing->SendOtaProgress(0, 100, 100);
    }

    Log::Write("Update download finished successfully. Rebooting in 3 seconds.");

    ev_timer rebootTimer;
    ev_timer_init(&rebootTimer, HandleRebootTimer, 3, 0);
    ev_timer_start(_loop, &rebootTimer);
  } else {
    // error happened while downloading OTA update
    if(_securePairing != nullptr) {
      _securePairing->SendOtaProgress(255, 0, 0);
    }
  }

  if(_securePairing != nullptr) {
    _securePairing->SetOtaUpdating(false);
  }

  ev_timer_stop(_loop, &_handleOtaTimer.timer);
  _isOtaUpdating = false;
  _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
}

void Daemon::OnOtaUpdatedRequest(std::string url) {
  if(_isOtaUpdating) {
    // handle
    return;
  }

  _isOtaUpdating = true;
  ev_timer_again(_loop, &_handleOtaTimer.timer);
  _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::UPDATING_OS);

  // remove progress files if exist
  Log::Write("Ota Update Initialized...");
  std::string stdout = "";
  (void) ExecCommand({"./bin/update-engine"}, stdout);

  Log::Write("Cleared files? %s", stdout.c_str());
  ExecCommandInBackground({"./bin/update-engine", url}, std::bind(&Daemon::HandleOtaUpdateExit, this, std::placeholders::_1, std::placeholders::_2));
}

void Daemon::OnPairingStatus(Anki::Cozmo::ExternalInterface::MessageEngineToGame message) {
  Anki::Cozmo::ExternalInterface::MessageEngineToGameTag tag = message.GetTag();

  switch(tag){
    case Anki::Cozmo::ExternalInterface::MessageEngineToGameTag::EnterPairing: {
      printf("Enter pairing: %hhu\n", tag);    
      if(_securePairing != nullptr) {
        _securePairing->SetIsPairing(true);
      }
      UpdateAdvertisement(true);
      _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::START_PAIRING);
      break;
    }
    case Anki::Cozmo::ExternalInterface::MessageEngineToGameTag::ExitPairing: {
      printf("Exit pairing: %hhu\n", tag);
      UpdateAdvertisement(false);
      if(_securePairing != nullptr) {
        _securePairing->StopPairing();
      }
      _engineMessagingClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
      break;
    }
    default: {
      printf("Unknown Tag: %hhu\n", tag);
      break;
    }
  }
}

void Daemon::HandleEngineTimer(struct ev_loop* loop, struct ev_timer* w, int revents) {
  ev_EngineTimerStruct* t = (ev_EngineTimerStruct*)w;
  bool connected = t->daemon->TryConnectToEngineServer();

  if(connected) {
    ev_timer_stop(loop, &t->timer);
    t->daemon->InitializeBleComms();
  }
}

void Daemon::HandleRebootTimer(struct ev_loop* loop, struct ev_timer* w, int revents) {
  Log::Write("Rebooting...");
  sync(); sync(); sync();
  reboot(LINUX_REBOOT_CMD_RESTART);
}

void Daemon::sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents)
{  
  struct ev_TimerStruct *wData = (struct ev_TimerStruct*)w;
  wData->signal->emit();
}

} // Switchboard
} // Anki

// ####################################################################################################################
// Entry Point
// ####################################################################################################################
static struct ev_signal sIntSig;
static struct ev_signal sTermSig;
static ev_timer sTimer;
static struct ev_loop* sLoop;
const static uint32_t kTick_s = 30;
std::unique_ptr<Anki::Switchboard::Daemon> _daemon;

static void ExitHandler(int status = 0) {
  // todo: smoothly handle termination
  _exit(status);
}

static void SignalCallback(struct ev_loop* loop, struct ev_signal* w, int revents)
{
  logi("Exiting for signal %d", w->signum);

  if(_daemon != nullptr) {
    _daemon->Stop();
  }

  ev_timer_stop(sLoop, &sTimer);
  ev_unloop(sLoop, EVUNLOOP_ALL);
  ExitHandler();
}

static void Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  // noop
}

int main() {
  sLoop = ev_default_loop(0);

  ev_signal_init(&sIntSig, SignalCallback, SIGINT);
  ev_signal_start(sLoop, &sIntSig);
  ev_signal_init(&sTermSig, SignalCallback, SIGTERM);
  ev_signal_start(sLoop, &sTermSig);
  
  // initialize daemon
  _daemon = std::make_unique<Anki::Switchboard::Daemon>(sLoop);
  _daemon->Start();

  // exit
  ev_timer_init(&sTimer, Tick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &sTimer);
  ev_loop(sLoop, 0);
  ExitHandler();
  return 0;
}
// ####################################################################################################################