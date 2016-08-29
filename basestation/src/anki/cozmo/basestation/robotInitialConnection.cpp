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

#include "anki/cozmo/basestation/robotInitialConnection.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/firmwareUpdater/firmwareUpdater.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include <json/json.h>

namespace Anki {
namespace Cozmo {


namespace RobotInitialConnectionConsoleVars
{
  CONSOLE_VAR(bool, kSkipFirmwareAutoUpdate, "Firmware", false);
}
  
using namespace ExternalInterface;
using namespace RobotInterface;
using namespace RobotInitialConnectionConsoleVars;
  
RobotInitialConnection::RobotInitialConnection(RobotID_t id, MessageHandler* messageHandler,
    IExternalInterface* externalInterface, uint32_t fwVersion, uint32_t fwTime)
: _id(id)
, _notified(false)
, _externalInterface(externalInterface)
, _fwVersion(fwVersion)
, _fwTime(fwTime)
, _validFirmware(false) // guilty until proven innocent
{
  if (_externalInterface == nullptr) {
    return;
  }

  auto handleFactoryFunc = std::bind(&RobotInitialConnection::HandleFactoryFirmware, this, std::placeholders::_1);
  AddSignalHandle(messageHandler->Subscribe(_id, RobotToEngineTag::factoryFirmwareVersion, handleFactoryFunc));

  auto handleFirmwareFunc = std::bind(&RobotInitialConnection::HandleFirmwareVersion, this, std::placeholders::_1);
  AddSignalHandle(messageHandler->Subscribe(_id, RobotToEngineTag::firmwareVersion, handleFirmwareFunc));
}

bool RobotInitialConnection::ShouldFilterMessage(RobotToEngineTag messageTag) const
{
  if (_validFirmware) {
    return false;
  }

  switch (messageTag) {
    // these messages are ok on outdated firmware
    case RobotToEngineTag::otaAck:
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

  switch (messageTag) {
      // these messages are ok on outdated firmware
    case EngineToRobotTag::otaWrite:
      return false;

    default:
      return true;
  }
}

bool RobotInitialConnection::HandleDisconnect()
{
  if (_notified || _externalInterface == nullptr) {
    return false;
  }

  PRINT_NAMED_INFO("RobotInitialConnection.HandleDisconnect", "robot connection failed");

  const auto result = RobotConnectionResult::ConnectionFailure;
  OnNotified(result);
  return true;
}

void RobotInitialConnection::HandleFactoryFirmware(const AnkiEvent<RobotToEngine>&)
{
  if (_notified || _externalInterface == nullptr) {
    return;
  }

  PRINT_NAMED_INFO("RobotInitialConnection.HandleFactoryFirmware", "robot has factory firmware");

  const auto result = RobotConnectionResult::OutdatedFirmware;
  OnNotified(result);
}

void RobotInitialConnection::HandleFirmwareVersion(const AnkiEvent<RobotToEngine>& message)
{
  if (_notified || _externalInterface == nullptr) {
    return;
  }

  const auto& fwData = message.GetData().Get_firmwareVersion().json;
  std::string jsonString{fwData.begin(), fwData.end()};
  Json::Reader reader;
  Json::Value headerData;
  reader.parse(jsonString, headerData);

  uint32_t robotVersion = headerData[FirmwareUpdater::kFirmwareVersionKey].asUInt();
  uint32_t robotTime = headerData[FirmwareUpdater::kFirmwareTimeKey].asUInt();

  // for firmware that came from a build server, a version number will be baked in,
  // but for local dev builds, the version field will equal the time field
  const bool robotHasDevFirmware = robotVersion == robotTime;
  const bool appHasDevFirmware = _fwVersion == _fwTime;

  // simulated robot will have special tag in json
  const bool robotIsSimulated = robotVersion == 0 && robotTime == 0 && !headerData["sim"].isNull();

  PRINT_NAMED_INFO("RobotInitialConnection.HandleFirmwareVersion", "robot firmware: %d%s%s (app: %d%s)", robotVersion,
    robotHasDevFirmware ? " (dev)" : "", robotIsSimulated ? " (SIM)" : "", _fwVersion, appHasDevFirmware ? " (dev)" : "");

  RobotConnectionResult result;
  if (robotIsSimulated || kSkipFirmwareAutoUpdate) {
    result = RobotConnectionResult::Success;
  }
  else if (robotHasDevFirmware != appHasDevFirmware) {
    // assume that release firmware on app takes precedence over dev firmware on robot,
    // and assume that someone with a dev app knows what they're doing if connecting to
    // a robot with release firmware
    result = RobotConnectionResult::OutdatedFirmware;
  }
  else {
    // either both are on dev or both are on release, so we can compare version numbers
    if (_fwVersion != robotVersion) {
      result = _fwVersion > robotVersion ? RobotConnectionResult::OutdatedFirmware : RobotConnectionResult::OutdatedApp;
    }
    else {
      result = RobotConnectionResult::Success;
    }
  }

  OnNotified(result);
}

void RobotInitialConnection::OnNotified(RobotConnectionResult result)
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
  _notified = true;
  ClearSignalHandles();
  
  _externalInterface->Broadcast(MessageEngineToGame{RobotConnectionResponse{_id, result}});
}

}
}
