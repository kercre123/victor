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
#include "clad/robotInterface/messageEngineToRobot.h"
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
, _robotMessageHandler(messageHandler)
, _fwVersion(fwVersion)
, _fwTime(fwTime)
, _validFirmware(false) // guilty until proven innocent
, _robotIsAvailable(false)
{
  if (_externalInterface == nullptr) {
    return;
  }

  auto handleFactoryFunc = std::bind(&RobotInitialConnection::HandleFactoryFirmware, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(_id, RobotToEngineTag::factoryFirmwareVersion, handleFactoryFunc));

  auto handleFirmwareFunc = std::bind(&RobotInitialConnection::HandleFirmwareVersion, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(_id, RobotToEngineTag::firmwareVersion, handleFirmwareFunc));
  
  auto handleAvailableFunc = std::bind(&RobotInitialConnection::HandleRobotAvailable, this, std::placeholders::_1);
  AddSignalHandle(_robotMessageHandler->Subscribe(_id, RobotToEngineTag::robotAvailable, handleAvailableFunc));
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
    case EngineToRobotTag::shutdownRobot:
      return false;

    default:
      return true;
  }
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
  
  Util::sEventF("robot.factory_firmware_version", {}, "0");

  const auto result = RobotConnectionResult::OutdatedFirmware;
  OnNotified(result, 0);
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
  if(!reader.parse(jsonString, headerData))
  {
    PRINT_NAMED_ERROR("RobotInitialConnection.HandleFirmwareVersion.ParseJson",
                      "Failed to parse header data from robot");
    // If parsing failed than report outdatedfirmware so the firmware gets updated to hopefully fix
    // what could be a malformed header
    
    OnNotified(RobotConnectionResult::OutdatedFirmware, 0);
    return;
  }

  std::string robotBuildType = headerData[FirmwareUpdater::kFirmwareBuildTypeKey].asString();

  if(robotBuildType == "FACTORY")
  {
    // Factory firmware's version is a string as it has an 'F' at the start
    const std::string& version = headerData[FirmwareUpdater::kFirmwareVersionKey].asString();
    
    if(version.at(0) != 'F')
    {
      PRINT_NAMED_ERROR("RobotInitialConnection.HandleFirmwareVersion.UnknownVersion",
                        "Robot has unexpected firmware version %s",
                        version.c_str());
    }
    else
    {
      PRINT_NAMED_INFO("RobotInitialConnection.HandleFirmwareVersion.FactoryFirmware",
                       "Robot has factory firmware version %s",
                       version.c_str());
    }
    
    Util::sEventF("robot.factory_firmware_version", {}, "%s", version.c_str());
    
    OnNotified(RobotConnectionResult::OutdatedFirmware, 0);
    return;
  }

  uint32_t robotVersion = headerData[FirmwareUpdater::kFirmwareVersionKey].asUInt();
  uint32_t robotTime = headerData[FirmwareUpdater::kFirmwareTimeKey].asUInt();

  // for firmware that came from a build server, a version number will be baked in,
  // but for local dev builds, the version field will equal the time field
  const bool robotHasDevFirmware = robotVersion == robotTime;
  const bool appHasDevFirmware = _fwVersion == _fwTime;

  // simulated robot will have special tag in json
  const bool robotIsSimulated = robotVersion == 0 && robotTime == 0 && !headerData["sim"].isNull();

  Util::sEventF("robot.firmware_version",
                {{DDATA, (headerData[FirmwareUpdater::kFirmwareTimeKey].asString() + std::string(".") + 
                          robotBuildType).c_str()}},
                "%u", robotVersion);
  Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "RobotInitialConnection.HandleFirmwareVersion",
                        {{DDATA, jsonString.c_str()}},
                        "robot firmware: %d%s%s%s (app: %d%s)", robotVersion, 
                        robotBuildType.c_str(),
                        robotHasDevFirmware ? " (dev)" : "",
                        robotIsSimulated ? " (SIM)" : "", _fwVersion, appHasDevFirmware ? " (dev)" : "");
  
  if (!_robotIsAvailable && !robotIsSimulated) {
    PRINT_NAMED_ERROR("RobotInitialConnection.HandleFirmwareVersion",
                      "Haven't gotten robot available message before firmware version");
    OnNotified(RobotConnectionResult::ConnectionFailure, 0);
    _validFirmware = false;
    return;
  }

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

  OnNotified(result, robotVersion);
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
    AddSignalHandle(_robotMessageHandler->Subscribe(_id, RobotToEngineTag::mfgId, [this, result, robotFwVersion] (const AnkiEvent<RobotToEngine>& message) {
      const auto& payload = message.GetData().Get_mfgId();
      _serialNumber = payload.esn;
      _bodyHWVersion = payload.hw_version;
      
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

      _robotMessageHandler->ConnectRobotToNeedsManager(_serialNumber);
    }));
    
    _robotMessageHandler->SendMessage(_id, RobotInterface::EngineToRobot{RobotInterface::GetManufacturingInfo{}});
  }
  else {
    SendConnectionResponse(result, robotFwVersion);
  }
}
  
void RobotInitialConnection::SendConnectionResponse(RobotConnectionResult result, uint32_t robotFwVersion)
{
  _notified = true;
  ClearSignalHandles();

  _externalInterface->Broadcast(MessageEngineToGame{RobotConnectionResponse{_id,
                                                                            result,
                                                                            robotFwVersion,
                                                                            _serialNumber,
                                                                            _bodyHWVersion,
                                                                            _bodyColor}});
}

}
}
