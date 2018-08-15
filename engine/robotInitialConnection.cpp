/**
 * File: robotInitialConnection
 *
 * Author: baustin
 * Created: 8/1/2016
 *
 * Description: Monitors initial events after robot connection to determine result to report
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/cozmoContext.h"
#include "engine/robotInitialConnection.h"
#include "engine/robotManager.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/utils/cozmoExperiments.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include <json/json.h>

#include "util/string/stringUtils.h"

namespace Anki {
namespace Vector {

// Anonymous namespace to hold onto whether we've forced update firmware yet for
// the robot that we've connected to. Only used in dev builds.
namespace {
  bool _robotForcedToFirmwareUpdate = false;
}

namespace RobotInitialConnectionConsoleVars
{
  CONSOLE_VAR(bool, kSkipFirmwareAutoUpdate, "Firmware", false);
  CONSOLE_VAR(bool, kAlwaysDoFirmwareUpdate, "Firmware", false);
}

using namespace ExternalInterface;
using namespace RobotInterface;
using namespace RobotInitialConnectionConsoleVars;

RobotInitialConnection::RobotInitialConnection(const CozmoContext* context)
: _notified(false)
, _externalInterface(context != nullptr ? context->GetExternalInterface() : nullptr)
, _context(context)
, _robotMessageHandler(context != nullptr ? context->GetRobotManager()->GetMsgHandler() : nullptr)
, _validFirmware(false) // guilty until proven innocent
, _robotIsAvailable(false)
{
  if (_externalInterface == nullptr) {
    return;
  }

  auto handleFactoryFunc = std::bind(&RobotInitialConnection::HandleFactoryFirmware, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(RobotToEngineTag::factoryFirmwareVersion, handleFactoryFunc));

  auto handleFirmwareFunc = std::bind(&RobotInitialConnection::HandleFirmwareVersion, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(RobotToEngineTag::firmwareVersion, handleFirmwareFunc));

  auto handleAvailableFunc = std::bind(&RobotInitialConnection::HandleRobotAvailable, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(RobotToEngineTag::robotAvailable, handleAvailableFunc));
}

bool RobotInitialConnection::ShouldFilterMessage(RobotToEngineTag messageTag) const
{
  if (_validFirmware) {
    return false;
  }

  switch (messageTag) {
    // these messages are ok on outdated firmware
    case RobotToEngineTag::robotAvailable:
    case RobotToEngineTag::factoryFirmwareVersion:
    case RobotToEngineTag::firmwareVersion:
      return false;

    default:
      return true;
  }
}

bool RobotInitialConnection::ShouldFilterMessage(EngineToRobotTag messageTag) const
{
  if (_validFirmware) {
    return false;
  }

  return true;
}

bool RobotInitialConnection::HandleDisconnect(RobotConnectionResult connectionResult)
{
  if (_notified || _externalInterface == nullptr) {
    return false;
  }

  PRINT_NAMED_INFO("RobotInitialConnection.HandleDisconnect", "robot connection failed due to %s", RobotConnectionResultToString(connectionResult));

  OnNotified(connectionResult, 0);
  return true;
}

void RobotInitialConnection::HandleFactoryFirmware(const AnkiEvent<RobotToEngine>&)
{
  if (_notified || _externalInterface == nullptr) {
    return;
  }

  PRINT_NAMED_INFO("RobotInitialConnection.HandleFactoryFirmware", "robot has factory firmware");

  Util::sInfoF("robot.factory_firmware_version", {}, "0");

  const auto result = RobotConnectionResult::OutdatedFirmware;
  OnNotified(result, 0);
}

void RobotInitialConnection::HandleFirmwareVersion(const AnkiEvent<RobotToEngine>& message)
{
  if (_notified || _externalInterface == nullptr) {
    return;
  }

  // TODO: VictorFirmwareUpdate
  // spoof the member variables as if a connection is successful
  // punting on this issue until Victor's firmware/engine update
  // flow is ready for development

  _robotForcedToFirmwareUpdate = false;
  _validFirmware               = true;
  if (!_robotIsAvailable) {
    PRINT_NAMED_ERROR("RobotInitialConnection.HandleFirmwareVersion.RobotUnavailable",
                      "Did not get RobotAvailable before FirmwareVersion message");
    OnNotified(RobotConnectionResult::ConnectionFailure, 0);
  } else {
    OnNotified(RobotConnectionResult::Success, 0);
  }
}

void RobotInitialConnection::HandleRobotAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _robotIsAvailable = true;
}

void RobotInitialConnection::OnNotified(RobotConnectionResult result, uint32_t robotFwVersion)
{
  switch (result) {
    case RobotConnectionResult::OutdatedFirmware:
    case RobotConnectionResult::OutdatedApp:
      _validFirmware = false;
      break;
    default:
      _validFirmware = true;
      break;
  }

  // If the connection completed successfully, ask for the robot ID and send the success once we get it
  if (RobotConnectionResult::Success == result) {
    AddSignalHandle(_robotMessageHandler->Subscribe(RobotToEngineTag::mfgId, [this, result, robotFwVersion] (const AnkiEvent<RobotToEngine>& message) {
      const auto& payload = message.GetData().Get_mfgId();
      _serialNumber = payload.esn;
      _bodyHWVersion = payload.hw_version;

      if (ANKI_DEV_CHEATS)
      {
        // Once we've successfully connected, reset our forced firmware update tracker
        _robotForcedToFirmwareUpdate = false;
      }

      const BodyColor bodyColor = static_cast<BodyColor>(payload.body_color);
      if(bodyColor <= BodyColor::UNKNOWN ||
         bodyColor >= BodyColor::COUNT ||
         bodyColor == BodyColor::RESERVED)
      {
        PRINT_NAMED_ERROR("RobotInitialConnection.HandleMfgID.InvalidBodyColor",
                          "Robot has invalid body color %d", payload.body_color);
      }
      else
      {
        _bodyColor = bodyColor;
      }

      SendConnectionResponse(result, robotFwVersion);

      _context->GetExperiments()->ReadLabAssignmentsFromRobot(_serialNumber);
    }));

    _robotMessageHandler->SendMessage(RobotInterface::EngineToRobot{RobotInterface::GetManufacturingInfo{}});
  }
  else {
    SendConnectionResponse(result, robotFwVersion);
  }
}

void RobotInitialConnection::SendConnectionResponse(RobotConnectionResult result, uint32_t robotFwVersion)
{
  _notified = true;
  ClearSignalHandles();

  _externalInterface->Broadcast(MessageEngineToGame{RobotConnectionResponse{result,
                                                                            robotFwVersion,
                                                                            _serialNumber,
                                                                            _bodyHWVersion,
                                                                            _bodyColor}});
}

}
}
