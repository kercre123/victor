#include <stdio.h> 
#include <sodium.h>
#include <signals/simpleSignal.hpp>
#include "libev/libev.h"
#include "anki-ble/log.h"
#include "anki-ble/bleClient.h"
#include "switchboardd/securePairing.h"

Anki::Switchboard::BleClient* bleClient;
struct ev_loop* defaultLoop;
Anki::TaskExecutor* taskExecutor;
Anki::Switchboard::SecurePairing* securePairing;
ev_timer timer;

void Test();
void StartBleComms();
void OnPinUpdated(std::string pin);
void OnConnected(int connId, Anki::Switchboard::IpcBleStream* stream);
void Tick(struct ev_loop* loop, struct ev_timer* w, int revents);

int main() {
  defaultLoop = ev_default_loop(0);
  taskExecutor = new Anki::TaskExecutor(defaultLoop);

  StartBleComms();

  ev_timer_init(&timer, Tick, 30, 30);
  ev_timer_start(defaultLoop, &timer);
  ev_loop(defaultLoop, 0);
}

void StartBleComms() {
  bleClient = new Anki::Switchboard::BleClient(defaultLoop);
  bleClient->OnConnectedEvent().SubscribeForever(OnConnected);
  bleClient->Connect();
  bleClient->StartAdvertising();
}

void OnConnected(int connId, Anki::Switchboard::IpcBleStream* stream) {
  taskExecutor->Wake([stream]() {
    printf("Connected to a BLE central.\n");

    if(securePairing == nullptr) {
      securePairing = new Anki::Switchboard::SecurePairing(stream, defaultLoop);
      securePairing->OnUpdatedPinEvent().SubscribeForever(OnPinUpdated);
      ///sWifiHandle = sSecurePairing->OnReceivedWifiCredentialsEvent().ScopedSubscribe(OnReceiveWifiCredentials);
    }
    
    // Initiate pairing process
    securePairing->BeginPairing();
  });
}

void OnPinUpdated(std::string pin) {
  Log::Write(pin.c_str());
}

void Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  // noop
}
