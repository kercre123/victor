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
#include "anki-websocket/websocketServer.h"
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
        _connectionFailureCounter(kFailureCountToLog),
        _taskExecutor(nullptr),
        _bleClient(nullptr),
        _securePairing(nullptr),
        _engineMessagingClient(nullptr)
      {}

      void Start();
      void Stop();
    
    private:
      const std::string kUpdateEngineDataPath = "/run/update-engine";
      const std::string kUpdateEngineExecPath = "/anki/bin";
  
      static void HandleEngineTimer(struct ev_loop* loop, struct ev_timer* w, int revents);
      static void HandleAnkibtdTimer(struct ev_loop* loop, struct ev_timer* w, int revents);
      static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
      void HandleReboot();

      using EvTimerSignal = Signal::Signal<void ()>;

      void Christen();
      void InitializeEngineComms();
      void InitializeBleComms();
      void OnConnected(int connId, INetworkStream* stream);
      void OnDisconnected(int connId, INetworkStream* stream);
      void OnBleIpcDisconnected();
      void OnPinUpdated(std::string pin);
      void OnOtaUpdatedRequest(std::string url);
      void OnEndPairing();
      void OnCompletedPairing();
      void OnPairingStatus(Anki::Cozmo::ExternalInterface::MessageEngineToGame message);
      bool TryConnectToEngineServer();
      bool TryConnectToAnkiBluetoothDaemon();
      void HandleOtaUpdateExit(int rc, const std::string& output);
      void HandleOtaUpdateProgress();
      void HandlePairingTimeout();
      int GetOtaProgress(uint64_t* progress, uint64_t* expected);

      Signal::SmartHandle _pinHandle;
      Signal::SmartHandle _otaHandle;
      Signal::SmartHandle _endHandle;
      Signal::SmartHandle _completedPairingHandle;

      Signal::SmartHandle _bleOnConnectedHandle;
      Signal::SmartHandle _bleOnDisconnectedHandle;
      Signal::SmartHandle _bleOnIpcPeerDisconnectedHandle;

      void UpdateAdvertisement(bool pairing);

      const uint8_t kOtaUpdateInterval_s = 1;
      const float kRetryInterval_s = 0.2f;
      const uint32_t kFailureCountToLog = 20;
      const uint32_t kPairingPreConnectionTimeout_s = 300;

      int _connectionId = -1;

      inline bool IsConnected() { return _connectionId != -1; }

      EvTimerSignal _otaUpdateTimerSignal;
      EvTimerSignal _pairingPreConnectionSignal;

      struct ev_loop* _loop;
      bool _isPairing;
      bool _isOtaUpdating;
      uint32_t _connectionFailureCounter;

      ev_timer _engineTimer;
      ev_timer _ankibtdTimer;

      struct ev_TimerStruct {
        ev_timer timer;
        EvTimerSignal* signal;
      } _handleOtaTimer;

      struct ev_TimerStruct _pairingTimer;

      std::unique_ptr<TaskExecutor> _taskExecutor;
      std::unique_ptr<BleClient> _bleClient;
      std::unique_ptr<SecurePairing> _securePairing;
      std::unique_ptr<WebsocketServer> _websocketServer;
      std::shared_ptr<EngineMessagingClient> _engineMessagingClient;
  };
}
}