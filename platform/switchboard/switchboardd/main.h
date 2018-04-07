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
#include "switchboardd/engineMessagingClient.h"

namespace Anki {
namespace Switchboard {
  enum OtaStatusCode {
    UNKNOWN     = 1,
    IN_PROGRESS = 2,
    COMPLETED   = 3,
    REBOOTING   = 4,
    ERROR       = 5,
  };

  class Daemon {
    public:
      Daemon(struct ev_loop* loop) :
        _loop(loop),
        _isPairing(false),
        _isOtaUpdating(false),
        _taskExecutor(nullptr),
        _bleClient(nullptr),
        _securePairing(nullptr),
        _engineMessagingClient(nullptr)
      {}

      void Start();
      void Stop();
    
    private:
      static void HandleEngineTimer(struct ev_loop* loop, struct ev_timer* w, int revents);
      static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
      void HandleReboot();

      using EvTimerSignal = Signal::Signal<void ()>;

      void InitializeEngineComms();
      void InitializeBleComms();
      void OnConnected(int connId, INetworkStream* stream);
      void OnDisconnected(int connId, INetworkStream* stream);
      void OnPinUpdated(std::string pin);
      void OnOtaUpdatedRequest(std::string url);
      void OnEndPairing();
      void OnPairingStatus(Anki::Cozmo::ExternalInterface::MessageEngineToGame message);
      bool TryConnectToEngineServer();
      void HandleOtaUpdateExit(int rc, const std::string& output);
      void HandleOtaUpdateProgress();
      int GetOtaProgress(uint64_t* progress, uint64_t* expected);

      Signal::SmartHandle _pinHandle;
      Signal::SmartHandle _otaHandle;
      Signal::SmartHandle _endHandle;

      void UpdateAdvertisement(bool pairing);

      const uint8_t kOtaUpdateInterval_s = 1;

      int _connectionId = -1;

      inline bool IsConnected() { return _connectionId != -1; }

      EvTimerSignal _otaUpdateTimerSignal;

      struct ev_loop* _loop;
      bool _isPairing;
      bool _isOtaUpdating;

      struct ev_EngineTimerStruct {
        ev_timer timer;
        Daemon* daemon;
      } _engineTimer;

      struct ev_TimerStruct {
        ev_timer timer;
        EvTimerSignal* signal;
      } _handleOtaTimer;

      std::unique_ptr<TaskExecutor> _taskExecutor;
      std::unique_ptr<BleClient> _bleClient;
      std::unique_ptr<SecurePairing> _securePairing;
      std::shared_ptr<EngineMessagingClient> _engineMessagingClient;
  };
}
}