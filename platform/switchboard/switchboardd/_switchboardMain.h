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
      static void Start();
      static void Stop();
    
    private:
      static void InitializeBle();
      static void Tick(struct ev_loop* loop, struct ev_timer* w, int revents);
      static void OnConnected(int connId, INetworkStream* stream);
      static void OnDisconnected(int connId, INetworkStream* stream);
      static void OnPinUpdated(std::string pin);

      // External Interfaces
      static void HandleButtonDoubleTap();  // todo
      static void DrawScreen();             // todo

      static int sConnectionId;

      static inline bool IsConnected() {
        return sConnectionId != -1;
      }

      const static uint32_t kTick_s = 30;
      static struct ev_loop* sLoop;
      static ev_timer sTimer;
      static Anki::TaskExecutor* sTaskExecutor;
      static BleClient* sBleClient;
      static SecurePairing* sSecurePairing;
  };
}
}