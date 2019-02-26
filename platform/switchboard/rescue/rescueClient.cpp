#include "rescue/rescueClient.h"
#include "rescue/miniFaceDisplay.h"

#include "ev++.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "cutils/properties.h"


namespace Anki {
namespace Switchboard {

bool RescueClient::Init()
{
  // TEMP: default fake fault code
  _faultCode = 42;

  // Get Robot Name
  char robotName[PROPERTY_VALUE_MAX] = {0};
  (void)property_get("anki.robot.name", robotName, "");
  _robotName = robotName;

  return true;
}

bool RescueClient::Connect()
{
  _isConnected = true;
  return true;
}

bool RescueClient::Disconnect()
{
  return true;
}

void RescueClient::StartPairing()
{
  // Send Enter pairing message
  Anki::Vector::SwitchboardInterface::EnterPairing enterPairing;
  using EMessage = Anki::Vector::ExternalInterface::MessageEngineToGame;
  EMessage msg;
  msg.Set_EnterPairing(std::move(enterPairing));
  _pairingStatusSignal.emit(msg);
}


void RescueClient::SendMessage(const Anki::Vector::ExternalInterface::MessageGameToEngine& message)
{
  /* noop */
}

void RescueClient::SetPairingPin(std::string pin)
{
  // BRC: Sent from daemon -> engine before sending ShowPairingStatus
  // use this to remember pin for display?
  _pin = pin;
}

void RescueClient::SendBLEConnectionStatus(bool connected)
{
  // BRC: Send from daemon -> engine to indicate that a client connected over BLE
  // Not sure if we need to take any action
}

void RescueClient::ShowPairingStatus(Anki::Vector::SwitchboardInterface::ConnectionStatus status)
{
  // BRC: send from daemon -> engine in order to change face display
  // display 

  using namespace Anki::Vector::SwitchboardInterface;
  switch(status) {
    case ConnectionStatus::NONE:
      break;
    case ConnectionStatus::START_PAIRING:
      break;
    case ConnectionStatus::SHOW_PRE_PIN:
      Anki::Vector::DrawShowPinScreen(_robotName, "######");
      break;
    case ConnectionStatus::SHOW_PIN:
      Anki::Vector::DrawShowPinScreen(_robotName, _pin);
      break;
    case ConnectionStatus::SETTING_WIFI:
    case ConnectionStatus::UPDATING_OS:
    case ConnectionStatus::UPDATING_OS_ERROR:
    case ConnectionStatus::WAITING_FOR_APP:
      Anki::Vector::DrawShowPinScreen(_robotName, "RESCUE");
      break;
    case ConnectionStatus::END_PAIRING:
      Anki::Vector::DrawFaultCode(_faultCode, _faultCodeRestart);
      break;
    default:
      break;
  }
}

//
// Ignore requests that would normally come from vic-engine
//

void RescueClient::HandleWifiScanRequest() { /* noop */ }

void RescueClient::HandleWifiConnectRequest(const std::string& ssid,
                                            const std::string& pwd,
                                            bool disconnectAfterConnection) { /* noop */ }

void RescueClient::HandleHasBleKeysRequest() { /* noop */ }

} // namespace Switchboard
} // namespace Anki