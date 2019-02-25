#include "rescue/rescueClient.h"

namespace Anki {
namespace Switchboard {

bool RescueClient::Init()
{
  return true;
}

bool RescueClient::Connect()
{
  return true;
}

void RescueClient::SendMessage(const Anki::Vector::ExternalInterface::MessageGameToEngine& message) {}

void RescueClient::SetPairingPin(std::string pin) {}

void RescueClient::SendBLEConnectionStatus(bool connected) {}

void RescueClient::ShowPairingStatus(Anki::Vector::SwitchboardInterface::ConnectionStatus status) {}

void RescueClient::HandleWifiScanRequest() {}

void RescueClient::HandleWifiConnectRequest(const std::string& ssid,
                                            const std::string& pwd,
                                            bool disconnectAfterConnection) {}

void RescueClient::HandleHasBleKeysRequest() {}

} // namespace Switchboard
} // namespace Anki