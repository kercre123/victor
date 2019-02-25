#ifndef PLATFORM_SWITCHBOARD_SWITCHBOARDD_RESCUECLIENT_H_
#define PLATFORM_SWITCHBOARD_SWITCHBOARDD_RESCUECLIENT_H_

#include "switchboardd/ISwitchboardCommandClient.h"

namespace Anki {
namespace Switchboard {

class RescueClient : public ISwitchboardCommandClient {
public:
  bool Init();
  bool Connect();
  bool Disconnect();
  void SendMessage(const Anki::Vector::ExternalInterface::MessageGameToEngine& message);
  void SetPairingPin(std::string pin);
  void SendBLEConnectionStatus(bool connected);
  void ShowPairingStatus(Anki::Vector::SwitchboardInterface::ConnectionStatus status);
  void HandleWifiScanRequest();
  void HandleWifiConnectRequest(const std::string& ssid,
                                const std::string& pwd,
                                bool disconnectAfterConnection);
  void HandleHasBleKeysRequest();
  EngineMessageSignal& OnReceivePairingStatus() {
    return _pairingStatusSignal;
  }
  EngineMessageSignal& OnReceiveEngineMessage() {
    return _engineMessageSignal;
  }
private:
  EngineMessageSignal _pairingStatusSignal;
  EngineMessageSignal _engineMessageSignal;
};

} // namespace Switchboard
} // namespace Anki

#endif // PLATFORM_SWITCHBOARD_SWITCHBOARDD_RESCUECLIENT_H_
