/**
 * File: behaviorPlaypenInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenInitChecks.h"

#include "engine/components/cliffSensorComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenInitChecks::BehaviorPlaypenInitChecks(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  IBehavior::SubscribeToTags(robot.GetID(),
                             {{RobotInterface::RobotToEngineTag::firmwareVersion,
                               RobotInterface::RobotToEngineTag::mfgId}});
}

Result BehaviorPlaypenInitChecks::InternalInitInternal(Robot& robot)
{
  
  if(robot.GetCliffSensorComponent().IsCliffDetectedStatusBitOn())
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CLIFF_UNEXPECTED, RESULT_FAIL);
  }
  
  if(robot.GetBatteryVoltage() < PlaypenConfig::kMinBatteryVoltage)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::BATTERY_TOO_LOW, RESULT_FAIL);
  }
  
  if(!(robot.IsOnCharger() && robot.IsCharging()))
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, RESULT_FAIL);
  }
  
  if(_failedToParseVersionHeader)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::PARSE_HEADER_FAILED, RESULT_FAIL);
  }
  
  if(robot.IsPhysical())
  {
    if(PlaypenConfig::kCheckFirmwareVersion &&
       (_fwVersion < PlaypenConfig::kMinFirmwareVersion || _fwBuildType != "FACTORY"))
    {
      PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::WRONG_FIRMWARE_VERSION, RESULT_FAIL);
    }
  }
  
  if(robot.GetBodyColor() < PlaypenConfig::kMinBodyColor)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::UNKNOWN_BODY_COLOR, RESULT_FAIL);
  }
  
  auto callback = [this](NVStorage::NVResult res) {
    if(res != NVStorage::NVResult::NV_OKAY)
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::NVSTORAGE_ERASE_FAILED);
    }
    
    _eraseComplete = true;
  };
  
  if(!robot.GetNVStorageComponent().WipeFactory(callback))
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::NVSTORAGE_SEND_FAILED, RESULT_FAIL);
  }
  
  robot.SendMessage(RobotInterface::EngineToRobot{RobotInterface::GetManufacturingInfo{}});
  AddTimer(PlaypenConfig::kMfgIDTimeout_ms, [this]() {PLAYPEN_SET_RESULT(FactoryTestResultCode::NO_BODY_VERSION_MESSAGE)});
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenInitChecks::InternalUpdateInternal(Robot& robot)
{
  if(_gotMfgID && _hwVersion < PlaypenConfig::kMinHardwareVersion)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::WRONG_BODY_HW_VERSION, BehaviorStatus::Failure);
  }
  
  if(_eraseComplete && _gotMfgID)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, BehaviorStatus::Complete);
  }
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenInitChecks::StopInternal(Robot& robot)
{
  // Note: I am explicitly not reseting things like _fwBuildType, _fwVersion, etc
  // due to the fact that those are set from a message that is automatically sent on connecting with
  // the robot. Can't set them again in the same session with the same robot
  
  _gotMfgID = false;
  _hwVersion = -1;
}

void BehaviorPlaypenInitChecks::AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot)
{
  const RobotInterface::RobotToEngineTag tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::firmwareVersion)
  {
    const auto& fwData = event.GetData().Get_firmwareVersion().json;
    std::string jsonString{fwData.begin(), fwData.end()};
    Json::Reader reader;
    Json::Value headerData;
    if(!reader.parse(jsonString, headerData))
    {
      _failedToParseVersionHeader = true;
      return;
    }
    
    if(headerData["version"].isUInt())
    {
      _fwVersion = headerData["version"].asUInt();
    }
    
    if(headerData["build"].isString())
    {
      _fwBuildType = headerData["build"].asString();
    }
  }
  else if(tag == RobotInterface::RobotToEngineTag::mfgId)
  {
    _gotMfgID = true;
    
    const auto& payload = event.GetData().Get_mfgId();
    _hwVersion = payload.hw_version;
  }
}

}
}


