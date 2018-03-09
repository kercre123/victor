/**
 * File: _switchboardMain.h
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
#include <stdlib.h>
#include "libev/libev.h"
#include "anki-ble/bleClient.h"
#include "switchboardd/securePairing.h"
#include "switchboardd/taskExecutor.h"

namespace Anki {
namespace Switchboard {
  class Daemon {
    public:
      Daemon(struct ev_loop* loop, ev_timer timer) :
        _loop(loop),
        _timer(timer)
      {}

      void Start();
      void Stop();
    
    private:
      void InitializeBle();
      void Tick(struct ev_loop* loop, struct ev_timer* w, int revents);
      void OnConnected(int connId, INetworkStream* stream);
      void OnDisconnected(int connId, INetworkStream* stream);
      void OnPinUpdated(std::string pin);

      // External Interfaces
      void HandleButtonDoubleTap();  // todo
      void DrawScreen();             // todo

      int _connectionId = -1;

      inline bool IsConnected() { return _connectionId != -1; }

      struct ev_loop* _loop;
      ev_timer _timer;
      std::unique_ptr<TaskExecutor> _taskExecutor;
      std::unique_ptr<BleClient> _bleClient;
      std::unique_ptr<SecurePairing> _securePairing;
  };
}
}