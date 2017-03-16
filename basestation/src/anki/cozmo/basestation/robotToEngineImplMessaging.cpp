/**
 * File: robotImplMessaging
 *
 * Author: damjan stulic
 * Created: 9/9/15
 *
 * Description:
 * robot class methods specific to message handling
 *
 * Copyright: Anki, inc. 2015
 *
 */

#include <opencv2/imgproc.hpp>

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/components/blockTapFilterComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/types/robotStatusAndActions.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/debug/messageDebugging.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeFstream.h"
#include "util/signals/signalHolder.h"
#include <functional>

// Whether or not to handle prox obstacle events
#define HANDLE_PROX_OBSTACLES 0

// Prints the IDs of the active blocks that are on but not currently
// talking to a robot whose rssi is less than this threshold.
// Prints roughly once/sec.
#define DISCOVERED_OBJECTS_RSSI_PRINT_THRESH 50

// Filter that makes chargers not discoverable
#define IGNORE_CHARGER_DISCOVERY 0

// Always play robot audio on device
#define ALWAYS_PLAY_ROBOT_AUDIO_ON_DEVICE 0

// How often do we send power level updates to DAS?
#define POWER_LEVEL_INTERVAL_SEC 600

namespace Anki {
namespace Cozmo {
  
using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;

RobotToEngineImplMessaging::RobotToEngineImplMessaging(Robot* robot) :
    _hasMismatchedEngineToRobotCLAD(false)
  , _hasMismatchedRobotToEngineCLAD(false)
  , _traceHandler(robot)
{
}

RobotToEngineImplMessaging::~RobotToEngineImplMessaging()
{
  
}

void RobotToEngineImplMessaging::InitRobotMessageComponent(RobotInterface::MessageHandler* messageHandler, RobotID_t robotId, Robot* const robot)
{
  using localHandlerType = void(RobotToEngineImplMessaging::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
  // Create a helper lambda for subscribing to a tag with a local handler
  auto doRobotSubscribe = [this, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
  };
  
  using localHandlerTypeWithRoboRef = void(RobotToEngineImplMessaging::*)(const AnkiEvent<RobotInterface::RobotToEngine>&, Robot* const);
  auto doRobotSubscribeWithRoboRef = [this, robotId, messageHandler, robot] (RobotInterface::RobotToEngineTag tagType, localHandlerTypeWithRoboRef handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1, robot)));
  };
  
  // bind to specific handlers in the robotImplMessaging class
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::printText,                      &RobotToEngineImplMessaging::HandlePrint);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::factoryFirmwareVersion,         &RobotToEngineImplMessaging::HandleFWVersionInfo);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::pickAndPlaceResult,             &RobotToEngineImplMessaging::HandlePickAndPlaceResult);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::activeObjectDiscovered,         &RobotToEngineImplMessaging::HandleActiveObjectDiscovered);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::activeObjectConnectionState,    &RobotToEngineImplMessaging::HandleActiveObjectConnectionState);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::activeObjectMoved,              &RobotToEngineImplMessaging::HandleActiveObjectMoved);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::activeObjectStopped,            &RobotToEngineImplMessaging::HandleActiveObjectStopped);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::activeObjectUpAxisChanged,      &RobotToEngineImplMessaging::HandleActiveObjectUpAxisChanged);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::goalPose,                       &RobotToEngineImplMessaging::HandleGoalPose);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotStopped,                   &RobotToEngineImplMessaging::HandleRobotStopped);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::cliffEvent,                     &RobotToEngineImplMessaging::HandleCliffEvent);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::potentialCliff,                 &RobotToEngineImplMessaging::HandlePotentialCliffEvent);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::proxObstacle,                              &RobotToEngineImplMessaging::HandleProxObstacle);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::image,                          &RobotToEngineImplMessaging::HandleImageChunk);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imageGyro,                      &RobotToEngineImplMessaging::HandleImageImuData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imuDataChunk,                   &RobotToEngineImplMessaging::HandleImuData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imuRawDataChunk,                &RobotToEngineImplMessaging::HandleImuRawData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::syncTimeAck,                    &RobotToEngineImplMessaging::HandleSyncTimeAck);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotPoked,                     &RobotToEngineImplMessaging::HandleRobotPoked);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotAvailable,                 &RobotToEngineImplMessaging::HandleRobotSetHeadID);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::firmwareVersion,                 &RobotToEngineImplMessaging::HandleFirmwareVersion);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorCalibration,               &RobotToEngineImplMessaging::HandleMotorCalibration);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorAutoEnabled,               &RobotToEngineImplMessaging::HandleMotorAutoEnabled);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::dockingStatus,                             &RobotToEngineImplMessaging::HandleDockingStatus);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::mfgId,                          &RobotToEngineImplMessaging::HandleRobotSetBodyID);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::defaultCameraParams,            &RobotToEngineImplMessaging::HandleDefaultCameraParams);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::objectPowerLevel,               &RobotToEngineImplMessaging::HandleObjectPowerLevel);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::objectAccel,                    &RobotToEngineImplMessaging::HandleObjectAccel);
  
  
  // lambda wrapper to call internal handler
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::state,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::state");
                                                       const RobotState& payload = message.GetData().Get_state();
                                                       robot->UpdateFullRobotState(payload);
                                                     }));
  
  
  
  // lambda for some simple message handling
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::animState,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::animState");
                                                       if (robot->GetTimeSynced()) {
                                                         robot->SetNumAnimationBytesPlayed(message.GetData().Get_animState().numAnimBytesPlayed);
                                                         robot->SetNumAnimationAudioFramesPlayed(message.GetData().Get_animState().numAudioFramesPlayed);
                                                         robot->SetEnabledAnimTracks(message.GetData().Get_animState().enabledAnimTracks);
                                                         robot->SetAnimationTag(message.GetData().Get_animState().tag);
                                                       }
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseStarted,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::rampTraverseStarted");
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a ramp.", robot->GetID());
                                                       robot->SetOnRamp(true);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseCompleted,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::rampTraverseCompleted");
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a ramp.", robot->GetID());
                                                       robot->SetOnRamp(false);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseStarted,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::bridgeTraverseStarted");
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a bridge.", robot->GetID());
                                                       //SetOnBridge(true);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseCompleted,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::bridgeTraverseCompleted");
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a bridge.", robot->GetID());
                                                       //SetOnBridge(false);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::chargerMountCompleted,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::chargerMountCompleted");
                                                       
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d charger mount %s.", robot->GetID(), message.GetData().Get_chargerMountCompleted().didSucceed ? "SUCCEEDED" : "FAILED" );
                                                       
                                                       if (message.GetData().Get_chargerMountCompleted().didSucceed) {
                                                         robot->SetPoseOnCharger();
                                                       }
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::mainCycleTimeError,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::mainCycleTimeError");
                                                       
                                                       const RobotInterface::MainCycleTimeError& payload = message.GetData().Get_mainCycleTimeError();
                                                       if (payload.numMainTooLongErrors > 0) {
                                                         PRINT_NAMED_WARNING("Robot.MainCycleTooLong", " %d Num errors: %d, Avg time: %d us", robot->GetID(), payload.numMainTooLongErrors, payload.avgMainTooLongTime);
                                                       }
                                                       if (payload.numMainTooLateErrors > 0) {
                                                         PRINT_NAMED_WARNING("Robot.MainCycleTooLate", "%d Num errors: %d, Avg time: %d us", robot->GetID(), payload.numMainTooLateErrors, payload.avgMainTooLateTime);
                                                       }
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::dataDump,
                                                     [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::dataDump");
                                                       
                                                       const RobotInterface::DataDump& payload = message.GetData().Get_dataDump();
                                                       char buf[payload.data.size() * 2 + 1];
                                                       FormatBytesAsHex((char *)payload.data.data(), (int)payload.data.size(), buf, (int)sizeof(buf));
                                                       PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageDataDump", "ID: %d, size: %zd, data: %s", robot->GetID(), payload.data.size(), buf);
                                                     }));
  
  if (robot->HasExternalInterface())
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*robot->GetExternalInterface(), *robot, GetSignalHandles());
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableDroneMode>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestRobotSettings>();
  }
}

void RobotToEngineImplMessaging::HandleMotorCalibration(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleMotorCalibration");
  
  const MotorCalibration& payload = message.GetData().Get_motorCalibration();
  PRINT_NAMED_INFO("HandleMotorCalibration.Recvd", "Motor %d, started %d, autoStarted %d", (int)payload.motorID, payload.calibStarted, payload.autoStarted);
  
  if (payload.calibStarted) {
    Util::sEventF("HandleMotorCalibration.Start",
                  {{DDATA, std::to_string(payload.autoStarted).c_str()}},
                  "%s", EnumToString(payload.motorID));
  } else {
    Util::sEventF("HandleMotorCalibration.Complete",
                  {{DDATA, std::to_string(payload.autoStarted).c_str()}},
                  "%s", EnumToString(payload.motorID));
  }
  
  if( payload.motorID == MotorID::MOTOR_LIFT && payload.calibStarted && robot->IsCarryingObject() ) {
    // if this was a lift calibration, we are no longer holding a cube
    const bool deleteObjects = true; // we have no idea what happened to the cube, so remove completely from the origin
    robot->SetCarriedObjectAsUnattached(deleteObjects);
  }
  
  if (payload.motorID == MotorID::MOTOR_HEAD) {
    robot->SetHeadCalibrated(!payload.calibStarted);
  }
  
  if (payload.motorID == MotorID::MOTOR_LIFT) {
    robot->SetLiftCalibrated(!payload.calibStarted);
  }
  
  robot->Broadcast(ExternalInterface::MessageEngineToGame(MotorCalibration(payload)));
}
  
void RobotToEngineImplMessaging::HandleMotorAutoEnabled(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleMotorAutoEnabled");
  
  const MotorAutoEnabled& payload = message.GetData().Get_motorAutoEnabled();
  PRINT_NAMED_INFO("HandleMotorAutoEnabled.Recvd", "Motor %d, enabled %d", (int)payload.motorID, payload.enabled);

  if (!payload.enabled) {
    // Burnout protection triggered.
    // Someobody is probably messing with the lift
    LOG_EVENT("HandleMotorAutoEnabled.MotorDisabled", "%s", EnumToString(payload.motorID));
  } else {
    LOG_EVENT("HandleMotorAutoEnabled.MotorEnabled", "%s", EnumToString(payload.motorID));
  }

  // This probably applies here as it does in HandleMotorCalibration.
  // Seems reasonable to expect whatever object the robot may have been carrying to no longer be there.
  if( payload.motorID == MotorID::MOTOR_LIFT && !payload.enabled && robot->IsCarryingObject() ) {
    const bool deleteObjects = true; // we have no idea what happened to the cube, so remove completely from the origin
    robot->SetCarriedObjectAsUnattached(deleteObjects);
  }
    
  robot->Broadcast(ExternalInterface::MessageEngineToGame(MotorAutoEnabled(payload)));
}

void RobotToEngineImplMessaging::HandleRobotSetHeadID(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotSetHeadID");
  
  const RobotInterface::RobotAvailable& payload = message.GetData().Get_robotAvailable();
  const auto model = payload.modelID;
  const auto headID = payload.robotID;
  
  // Set DAS Global on all messages
  char string_id[32] = {};
  snprintf(string_id, sizeof(string_id), "0xbeef%04x%08x", model, headID);
  Anki::Util::sSetGlobal(DGROUP, string_id);
  
  // This should be definition always have a phys ID
  Anki::Util::sEvent("robot.handle_robot_set_head_id", {{DDATA,string_id}}, string_id);
  
  robot->SetHeadSerialNumber(headID);
  robot->SetModelNumber(model);
}
  
void RobotToEngineImplMessaging::HandleRobotSetBodyID(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotSetBodyID");
  
  const RobotInterface::ManufacturingID& payload = message.GetData().Get_mfgId();
  const auto hwVersion = payload.hw_version;
  const auto bodyID = payload.esn;
  
  // Set DAS Global on all messages
  char string_id[32] = {};
  snprintf(string_id, sizeof(string_id), "0xbeef%08x%08x", hwVersion, bodyID);
  
  Anki::Util::sSetGlobal(DPHYS, string_id);
  Anki::Util::sEvent("robot.handle_robot_set_body_id", {{DDATA,string_id}}, string_id);
  
  robot->SetBodySerialNumber(bodyID);
  robot->SetHWVersion(hwVersion);
}
  
void RobotToEngineImplMessaging::HandleFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  // Extract sim flag from json
  const auto& fwData = message.GetData().Get_firmwareVersion().json;
  std::string jsonString{fwData.begin(), fwData.end()};
  Json::Reader reader;
  Json::Value headerData;
  if(!reader.parse(jsonString, headerData))
  {
    return;
  }
  
  // simulated robot will have special tag in json
  const bool robotIsPhysical = headerData["sim"].isNull();

  PRINT_NAMED_INFO("RobotIsPhysical", "%d", robotIsPhysical);
  robot->SetPhysicalRobot(robotIsPhysical);
  
  // Update robot audio output source
  auto outputSource = (robotIsPhysical && !ALWAYS_PLAY_ROBOT_AUDIO_ON_DEVICE) ?
                      Audio::RobotAudioClient::RobotAudioOutputSource::PlayOnRobot :
                      Audio::RobotAudioClient::RobotAudioOutputSource::PlayOnDevice;
  robot->GetRobotAudioClient()->SetOutputSource(outputSource);
}
  

void RobotToEngineImplMessaging::HandlePrint(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandlePrint");
  const RobotInterface::PrintText& payload = message.GetData().Get_printText();
  printf("ROBOT-PRINT (%d): %s", robot->GetID(), payload.text.c_str());
}

void RobotToEngineImplMessaging::HandleFWVersionInfo(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleFWVersionInfo");
  
  static_assert(decltype(RobotInterface::FWVersionInfo::toRobotCLADHash)().size() == sizeof(messageEngineToRobotHash), "Incorrect sizes in CLAD version mismatch message");
  static_assert(decltype(RobotInterface::FWVersionInfo::toEngineCLADHash)().size() == sizeof(messageRobotToEngineHash), "Incorrect sizes in CLAD version mismatch message");
  
  _factoryFirmwareVersion = message.GetData().Get_factoryFirmwareVersion();

  std::string robotEngineToRobotStr;
  std::string engineEngineToRobotStr;
  if (memcmp(_factoryFirmwareVersion.toRobotCLADHash.data(), messageEngineToRobotHash, _factoryFirmwareVersion.toRobotCLADHash.size())) {
    robotEngineToRobotStr = Anki::Util::ConvertMessageBufferToString(_factoryFirmwareVersion.toRobotCLADHash.data(), static_cast<uint32_t>(_factoryFirmwareVersion.toRobotCLADHash.size()), Anki::Util::EBytesToTextType::eBTTT_Hex);
    engineEngineToRobotStr = Anki::Util::ConvertMessageBufferToString(messageEngineToRobotHash, sizeof(messageEngineToRobotHash), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    PRINT_NAMED_WARNING("RobotFirmware.VersionMismatch", "Engine to Robot CLAD version hash mismatch. Robot's EngineToRobot hash = %s. Engine's EngineToRobot hash = %s.", robotEngineToRobotStr.c_str(), engineEngineToRobotStr.c_str());
    
    _hasMismatchedEngineToRobotCLAD = true;
  }
  
  std::string robotRobotToEngineStr;
  std::string engineRobotToEngineStr;
  
  if (memcmp(_factoryFirmwareVersion.toEngineCLADHash.data(), messageRobotToEngineHash, _factoryFirmwareVersion.toEngineCLADHash.size())) {

    robotRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(_factoryFirmwareVersion.toEngineCLADHash.data(), static_cast<uint32_t>(_factoryFirmwareVersion.toEngineCLADHash.size()), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    engineRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(messageRobotToEngineHash, sizeof(messageRobotToEngineHash), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    PRINT_NAMED_WARNING("RobotFirmware.VersionMismatch", "Robot to Engine CLAD version hash mismatch. Robot's RobotToEngine hash = %s. Engine's RobotToEngine hash = %s.", robotRobotToEngineStr.c_str(), engineRobotToEngineStr.c_str());
    
    _hasMismatchedRobotToEngineCLAD = true;
  }
  
  if (_hasMismatchedEngineToRobotCLAD || _hasMismatchedRobotToEngineCLAD) {
    robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::EngineRobotCLADVersionMismatch(_hasMismatchedEngineToRobotCLAD,
                                                                                                              _hasMismatchedRobotToEngineCLAD,
                                                                                                              engineEngineToRobotStr,
                                                                                                              engineRobotToEngineStr,
                                                                                                              robotEngineToRobotStr,
                                                                                                              robotRobotToEngineStr)));
  }
}

void RobotToEngineImplMessaging::HandlePickAndPlaceResult(const AnkiEvent<RobotInterface::RobotToEngine>& message,
                                                          Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandlePickAndPlaceResult");
  
  const PickAndPlaceResult& payload = message.GetData().Get_pickAndPlaceResult();
  const char* successStr = (payload.didSucceed ? "succeeded" : "failed");
  
  robot->SetLastPickOrPlaceSucceeded(payload.didSucceed);
  
  // Note: this returns the vision system to whatever mode it was in before
  // it was docking/tracking
  robot->GetVisionComponent().EnableMode(VisionMode::Tracking, false);
  
  switch(payload.blockStatus)
  {
    case BlockStatus::NO_BLOCK:
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.NoBlock",
                       "Robot %d reported it %s doing something without a block. Stopping docking and turning on Look-for-Markers mode.", robot->GetID(), successStr);
      break;
    }
    case BlockStatus::BLOCK_PLACED:
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.BlockPlaced",
                       "Robot %d reported it %s placing block. Stopping docking and turning on Look-for-Markers mode.", robot->GetID(), successStr);
    
      if(payload.didSucceed) {
        robot->SetCarriedObjectAsUnattached();
      }
      
      robot->GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
      
      break;
    }
    case BlockStatus::BLOCK_PICKED_UP:
    {
      const char* resultStr = EnumToString(payload.result);
      
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.BlockPickedUp",
                       "Robot %d reported it %s picking up block with %s. Stopping docking and turning on Look-for-Markers mode.", robot->GetID(), successStr, resultStr);
    
      if(payload.didSucceed) {
        robot->SetDockObjectAsAttachedToLift();
      }

      break;
    }
  }
}

void RobotToEngineImplMessaging::HandleDockingStatus(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ANKI_CPU_PROFILE("Robot::HandleDockingStatus");
  
  // TODO: Do something with the docking status message like play sound or animation
  //const DockingStatus& payload = message.GetData().Get_dockingStatus();
  
  // Log event to help us track whether backup or "Hanns Manuever" is being used
  LOG_EVENT("robot.docking.status", "%s", EnumToString(message.GetData().Get_dockingStatus().status));
}

void RobotToEngineImplMessaging::HandleActiveObjectDiscovered(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectDiscovered");
  
  const ObjectDiscovered payload = message.GetData().Get_activeObjectDiscovered();
  
  // Check object type
  ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(payload.device_type);
  switch(objType) {
    case ObjectType::Charger_Basic:
    {
      if (IGNORE_CHARGER_DISCOVERY) {
        return;
      }
      break;
    }
    case ObjectType::Unknown:
    {
      PRINT_NAMED_WARNING("Robot.HandleActiveObjectDiscovered.UnknownType",
                          "FactoryID: 0x%x, device_type: 0x%hx",
                          payload.factory_id, payload.device_type);
      return;
    }
    default:
      break;
  }
  
  robot->SetDiscoveredObjects(payload.factory_id, objType, payload.rssi, robot->GetLastMsgTimestamp());  // Not super accurate, but this doesn't need to be
  
  if (robot->GetEnableDiscoveredObjectsBroadcasting()) {
    if (payload.rssi < DISCOVERED_OBJECTS_RSSI_PRINT_THRESH) {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectDiscovered.ObjectDiscovered",
                       "Type: %s, FactoryID 0x%x, rssi %d, (currTime %d)",
                       EnumToString(objType), payload.factory_id, payload.rssi, robot->GetLastMsgTimestamp());
    }
    
    // Send ObjectAvailable to game
    ExternalInterface::ObjectAvailable m(payload.factory_id, objType, payload.rssi);
    robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(m)));
  }
}

void RobotToEngineImplMessaging::HandleActiveObjectConnectionState(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectConnectionState");
  
  ObjectConnectionState payload = message.GetData().Get_activeObjectConnectionState();
  ObjectID objID;
  
  // Do checking here that the unsigned number we get as ActiveID (specified as payload.objectID) is actually less
  // than the max slot we're supposed to have as ActiveID. Extra checking here is necessary since the number is unsigned
  // and we do allow a negative ActiveID when calling AddActiveObject elsewhere, for adding the charger.
  if(payload.objectID >= Util::numeric_cast<uint32_t>(ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS)) {
    DEV_ASSERT(false, "Robot.HandleActiveObjectConnectionState.InvalidActiveID");
    return;
  }
  
  ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(payload.device_type);
  if (payload.connected)
  {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"connected"}}, "0x%x,%s",
                        payload.factoryID, EnumToString(payload.device_type));

    // Add active object to blockworld
    objID = robot->GetBlockWorld().AddConnectedActiveObject(payload.objectID, payload.factoryID, payload.device_type);
    if (objID.IsSet()) {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Connected",
                       "Object %d (activeID %d, factoryID 0x%x, device_type 0x%hx)",
                       objID.GetValue(), payload.objectID, payload.factoryID, payload.device_type);
      
      // do bookkeeping in robot
      robot->HandleConnectedToObject(payload.objectID, payload.factoryID, objType);
    }
  } else {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"disconnected"}}, "0x%x,%s",
                        payload.factoryID, EnumToString(payload.device_type));

    // Remove active object from blockworld if it exists, and remove all instances in all origins
    objID = robot->GetBlockWorld().RemoveConnectedActiveObject(payload.objectID);
    if ( objID.IsSet() )
    {
      // do bookkeeping in robot
      robot->HandleDisconnectedFromObject(payload.objectID, payload.factoryID, objType);
    }
  }
  
  PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Recvd", "FactoryID 0x%x, connected %d",
                   payload.factoryID, payload.connected);
  
  // Viz info
  robot->GetContext()->GetVizManager()->SendObjectConnectionState(payload.objectID, objType, payload.connected);
  
  // TODO: arguably blockworld should do this, because when do we want to remove/add objects and not notify?
  if (objID.IsSet()) {
    // Update the objectID to be blockworld ID
    payload.objectID = objID.GetValue();
    
    // Forward on to game
    robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectConnectionState(payload)));
  }
}

  
// Helpers used by the shared templated ObjectMovedOrStoppedHelper() method below,
// for each type of message.
template<class PayloadType> static const char* GetEventPrefix();
template<> inline const char* GetEventPrefix<ObjectMoved>() { return "Robot.ActiveObjectMoved."; }
template<> inline const char* GetEventPrefix<ObjectStoppedMoving>() { return "Robot.ActiveObjectStopped."; }

static inline const char* GetAxisString(const ObjectMoved& payload) { return EnumToString(payload.axisOfAccel); }
static inline const char* GetAxisString(const ObjectStoppedMoving& payload) { return "<unknown>"; }

static inline const float GetXAccelVal(const ObjectMoved& payload) { return payload.accel.x; }
static inline const float GetXAccelVal(const ObjectStoppedMoving& payload) { return 0; }

static inline const float GetYAccelVal(const ObjectMoved& payload) { return payload.accel.y; }
static inline const float GetYAccelVal(const ObjectStoppedMoving& payload) { return 0; }

static inline const float GetZAccelVal(const ObjectMoved& payload) { return payload.accel.z; }
static inline const float GetZAccelVal(const ObjectStoppedMoving& payload) { return 0; }
  
template<class PayloadType> static bool GetIsMoving();
template<> inline bool GetIsMoving<ObjectMoved>() { return true; }
template<> inline bool GetIsMoving<ObjectStoppedMoving>() { return false; }

  
// Shared helper we can use for Moved or Stopped messages
template<class PayloadType>
static void ObjectMovedOrStoppedHelper(Robot* const robot, PayloadType payload)
{
  auto activeID = payload.objectID;
  
  const std::string eventPrefix = GetEventPrefix<PayloadType>();

  // if we find an object with that activeID, its objectID will be here
  ObjectID matchedObjectID;
  
# define MAKE_EVENT_NAME(__str__) (eventPrefix + __str__).c_str()
  
  // find active object by activeID
  ObservableObject* connectedObj = robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(activeID);
  if ( nullptr == connectedObj ) {
    PRINT_NAMED_WARNING(MAKE_EVENT_NAME("UnknownActiveID"),
                        "Could not find match for active object ID %d", payload.objectID);
  }
  else
  {
    // Only do this stuff once, since these checks should be the same across all frames. Use connected instance
    if( connectedObj->GetID() == robot->GetCharger() )
    {
      PRINT_NAMED_INFO(MAKE_EVENT_NAME("Charger"), "Charger sending garbage move messages");
      return;
    }
  
    DEV_ASSERT(connectedObj->IsActive(), MAKE_EVENT_NAME("NonActiveObject"));
  
    PRINT_NAMED_INFO(MAKE_EVENT_NAME("ObjectMovedOrStopped"),
                     "ObjectID: %d (Active ID %d), type: %s, axisOfAccel: %s, accel: %f %f %f, time: %d ms",
                     connectedObj->GetID().GetValue(), connectedObj->GetActiveID(),
                     EnumToString(connectedObj->GetType()), GetAxisString(payload),
                     GetXAccelVal(payload), GetYAccelVal(payload), GetZAccelVal(payload), payload.timestamp );
    
    const bool shouldIgnoreMovement = robot->GetBlockTapFilter().ShouldIgnoreMovementDueToDoubleTap(connectedObj->GetID());
    if(shouldIgnoreMovement && GetIsMoving<PayloadType>())
    {
      PRINT_NAMED_INFO(MAKE_EVENT_NAME("IgnoringMessage"),
                       "Waiting for double tap id:%d ignoring movement message",
                       connectedObj->GetID().GetValue());
      return;
    }
    
    // for later notification
    matchedObjectID = connectedObj->GetID();
    
    // update Moving flag of connected object when it changes
    const bool wasMoving = connectedObj->IsMoving();
    const bool isMovingNow = GetIsMoving<PayloadType>();
    if ( wasMoving != isMovingNow ) {
      connectedObj->SetIsMoving(GetIsMoving<PayloadType>(), payload.timestamp);
      robot->GetContext()->GetVizManager()->SendObjectMovingState(activeID, connectedObj->IsMoving());
    }
  }
  
  // -- Update located instances
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID. We also need to consider all pose states and origin frames.
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn([activeID](const ObservableObject* object) {
    return object->IsActive() && object->GetActiveID() == activeID;
  });
  
  std::vector<ObservableObject *> matchingObjects;
  robot->GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects);
  
  for(ObservableObject* object : matchingObjects)
  {
    assert(object != nullptr); // FindMatchingObjects should not return nullptrs
    
    if(object->GetID() != matchedObjectID)
    {
      PRINT_NAMED_WARNING(MAKE_EVENT_NAME("ActiveObjectInDifferentFramesWithDifferentIDs"),
                          "First object=%d in '%s'. This object=%d in '%s'.",
                          matchingObjects.front()->GetID().GetValue(),
                          matchingObjects.front()->GetPose().FindOrigin().GetName().c_str(),
                          object->GetID().GetValue(),
                          object->GetPose().FindOrigin().GetName().c_str());
    }
    
    // We expect carried objects to move, so don't mark them as dirty/inaccurate.
    // Their pose state should remain accurate/known because they are attached to
    // the lift. I'm leaving this a separate check from the decision about broadcasting
    // the movement, in case we want to easily remove the checks above but keep this one.
    const bool isCarryingObject = robot->IsCarryingObject(object->GetID());
    if(object->IsPoseStateKnown() && !isCarryingObject)
    {
      // Once an object moves, we can no longer use it for localization because
      // we don't know where it is anymore. Next time we see it, relocalize it
      // relative to robot's pose estimate. Then we can use it for localization
      // again.
      robot->GetObjectPoseConfirmer().MarkObjectDirty(object);
    }
    
    const bool wasMoving = object->IsMoving();
    const bool isMovingNow = GetIsMoving<PayloadType>();
    if ( wasMoving != isMovingNow )
    {
      // Set moving state of object (in any frame)
      object->SetIsMoving(GetIsMoving<PayloadType>(), payload.timestamp);
    }
    
  } // for(ObservableObject* object : matchingObjects)
  
  if ( matchedObjectID.IsSet() )
  {
    // Don't notify game about objects being carried that have moved, since we expect
    // them to move when the robot does.
    // TODO: Consider broadcasting carried object movement if the robot is _not_ moving
    //
    // Don't notify game about moving objects that are being docked with, because
    // we expect those to move if/when we bump them. But we still mark them as dirty/inaccurate
    // below because they have in fact moved and we wouldn't want to localize with them.
    //
    // TODO: Consider not filtering these out and letting game ignore them somehow
    //       - Option 1: give game access to dockingID so it can do this same filtering
    //       - Option 2: add a "wasDocking" flag to the ObjectMoved/Stopped message
    //       - Option 3: add a new ObjectMovedWhileDocking message
    //
    const bool isDockingObject = (connectedObj->GetID() == robot->GetDockObject());
    const bool isCarryingObject = robot->IsCarryingObject(connectedObj->GetID());
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = matchedObjectID;
    payload.robotID = robot->GetID();
    
    if(!isDockingObject && !isCarryingObject)
    {
      robot->Broadcast(ExternalInterface::MessageEngineToGame(PayloadType(payload)));
    }
  }
  else
  {
    PRINT_NAMED_WARNING("ObjectMovedOrStoppedHelper.UnknownActiveID",
                        "Could not find match for active object ID %d", activeID);
  }
  
# undef MAKE_EVENT_NAME
  
} // ObjectMovedOrStoppedHelper()

void RobotToEngineImplMessaging::HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectMoved");
  
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectMoved payload = message.GetData().Get_activeObjectMoved();
  
  ObjectMovedOrStoppedHelper(robot, payload);
  
}

void RobotToEngineImplMessaging::HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectStopped");
  
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectStoppedMoving payload = message.GetData().Get_activeObjectStopped();
  
  ObjectMovedOrStoppedHelper(robot, payload);
}


void RobotToEngineImplMessaging::HandleActiveObjectUpAxisChanged(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectUpAxisChanged");

  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectUpAxisChanged payload = message.GetData().Get_activeObjectUpAxisChanged();

  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID. We also need to consider all pose states and origin frames.
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn([&payload](const ObservableObject* object) {
    return object->IsActive() && object->GetActiveID() == payload.objectID;
  });

  std::vector<ObservableObject *> matchingObjects;
  robot->GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects);

  if(matchingObjects.empty())
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // maybe we do not have located instances, there should be a connected one though
      ActiveID activeID = payload.objectID;
      const bool isConnected = ( nullptr != robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(activeID) );
      if ( !isConnected ) {
        PRINT_NAMED_WARNING("Robot.HandleActiveObjectUpAxisChanged.UnknownActiveID",
                            "Could not find match for active object ID %d", payload.objectID);
      }
    }
  
    return;
  }

  // Just use the first matching object since they will all be the same (matching ObjectIDs, ActiveIDs, FactoryIDs)
  ObservableObject* object = matchingObjects.front();
  
  PRINT_NAMED_INFO("Robot.HandleActiveObjectUpAxisChanged.UpAxisChanged",
                   "Type: %s, ObjectID: %d, UpAxis: %s",
                   EnumToString(object->GetType()),
                   object->GetID().GetValue(),
                   EnumToString(payload.upAxis));
  
  // Viz update
  robot->GetContext()->GetVizManager()->SendObjectUpAxisState(payload.objectID, payload.upAxis);
  
  // Update the ID to be the blockworld ID before broadcasting
  payload.objectID = object->GetID();
  payload.robotID = robot->GetID();
  robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectUpAxisChanged(payload)));
}

void RobotToEngineImplMessaging::HandleGoalPose(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleGoalPose");
  
  const GoalPose& payload = message.GetData().Get_goalPose();
  Anki::Pose3d p(payload.pose.angle, Z_AXIS_3D(),
                 Vec3f(payload.pose.x, payload.pose.y, payload.pose.z));
  //PRINT_INFO("Goal pose: x=%f y=%f %f deg (%d)", msg.pose_x, msg.pose_y, RAD_TO_DEG(msg.pose_angle), msg.followingMarkerNormal);
  if (payload.followingMarkerNormal) {
    robot->GetContext()->GetVizManager()->DrawPreDockPose(100, p, ::Anki::NamedColors::RED);
  } else {
    robot->GetContext()->GetVizManager()->DrawPreDockPose(100, p, ::Anki::NamedColors::GREEN);
  }
}

void RobotToEngineImplMessaging::HandleRobotStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotStopped");
  
  RobotInterface::RobotStopped payload = message.GetData().Get_robotStopped();
  Util::sEventF("RobotImplMessaging.HandleRobotStopped",
                {{DDATA, std::to_string(robot->GetCliffRunningVar()).c_str()}},
                "%d", payload.reason);
  
  robot->EvaluateCliffSuspiciousnessWhenStopped();
  
  // This is a somewhat overloaded use of enableCliffSensor, but currently only cliffs
  // trigger this RobotStopped message so it's not too crazy.
  if( !(robot->IsCliffSensorEnabled()) ) {
    return;
  }
  
  // Stop whatever we were doing
  robot->GetActionList().Cancel();
  
  // Forward on with EngineToGame event
  robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotStopped()));
}
  
void RobotToEngineImplMessaging::HandlePotentialCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandlePotentialCliffEvent");
  
  if(robot->GetIsCliffReactionDisabled()){
    IActionRunner* action = new TriggerLiftSafeAnimationAction(*robot, AnimationTrigger::DroneModeCliffEvent);
    robot->GetActionList().QueueAction(QueueActionPosition::NOW, action);
  }else{
    PRINT_NAMED_WARNING("Robot.HandlePotentialCliffEvent", "Got potential cliff message but not in drone mode");
    robot->GetMoveComponent().StopAllMotors();
    robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  }
}

void RobotToEngineImplMessaging::HandleCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleCliffEvent");
  
  CliffEvent cliffEvent = message.GetData().Get_cliffEvent();
  // always listen to events which say we aren't on a cliff, but ignore ones which say we are (so we don't
  // get "stuck" om a cliff
  if( !(robot->IsCliffSensorEnabled()) && cliffEvent.detected ) {
    return;
  }
  
  if (cliffEvent.detected) {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Detected", "at %f,%f",
                     cliffEvent.x_mm, cliffEvent.y_mm);
    
    // Add cliff obstacle
    Pose3d cliffPose(cliffEvent.angle_rad, Z_AXIS_3D(), {cliffEvent.x_mm, cliffEvent.y_mm, 0}, robot->GetWorldOrigin());
    robot->GetBlockWorld().AddCliff(cliffPose);
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Undetected", "");
  }
  
  robot->SetCliffDetected(cliffEvent.detected);
  
  // Forward on with EngineToGame event
  robot->Broadcast(ExternalInterface::MessageEngineToGame(CliffEvent(cliffEvent)));
}

void RobotToEngineImplMessaging::HandleProxObstacle(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
#if(HANDLE_PROX_OBSTACLES)
  ANKI_CPU_PROFILE("Robot::HandleProxObstacle");
  
  ProxObstacle proxObs = message.GetData().Get_proxObstacle();
  const bool obstacleDetected = proxObs.distance_mm < FORWARD_COLLISION_SENSOR_LENGTH_MM;
  if ( obstacleDetected )
  {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleProxObstacle.Detected", "at dist %d mm",
                     proxObs.distance_mm);
    
    // Compute location of obstacle
    // NOTE: This should actually depend on a historical pose, but this is all changing eventually anyway...
    f32 heading = GetPose().GetRotationAngle<'Z'>().ToFloat();
    Vec3f newPt(GetPose().GetTranslation());
    newPt.x() += proxObs.distance_mm * cosf(heading);
    newPt.y() += proxObs.distance_mm * sinf(heading);
    Pose3d obsPose(heading, Z_AXIS_3D(), newPt, GetWorldOrigin());
    
    // Add prox obstacle
    _blockWorld.AddProxObstacle(obsPose);
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleProxObstacle.Undetected", "max len %d mm", proxObs.distance_mm);
  }
  
  // always update the value
  SetForwardSensorValue(proxObs.distance_mm);
  
#endif
}


// For processing image chunks arriving from robot.
// Sends complete images to VizManager for visualization (and possible saving).
void RobotToEngineImplMessaging::HandleImageChunk(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImageChunk");
  
  // Ignore images if robot has not yet acknowledged time sync
  if (!(robot->GetTimeSynced())) {
    return;
  }
  
  const ImageChunk& payload = message.GetData().Get_image();
  
  const bool isImageReady = robot->GetEncodedImage().AddChunk(payload);
  
  // Forward the image chunks over external interface if image send mode is not OFF
  if (robot->GetContext()->GetExternalInterface() != nullptr && robot->GetImageSendMode() != ImageSendMode::Off && !ShouldIgnoreMultipleImages())
  {
    // we don't want to start sending right in the middle of an image, wait until we hit payload 0
    // before starting to send.
    if(payload.chunkId == 0 || robot->GetLastSentImageID() == payload.imageId) {
      robot->SetLastSentImageID(payload.imageId);
      
      ExternalInterface::MessageEngineToGame msgWrapper;
      msgWrapper.Set_ImageChunk(payload);
      robot->GetContext()->GetExternalInterface()->Broadcast(msgWrapper);
      
      const bool wasLastChunk = payload.chunkId == payload.imageChunkCount-1;
      
      if(wasLastChunk && robot->GetImageSendMode() == ImageSendMode::SingleShot) {
        // We were just in single-image send mode, and the image got sent, so
        // go back to "off". (If in stream mode, stay in stream mode.)
        robot->SetImageSendMode(ImageSendMode::Off);
      }
    }
  }
  
  // Forward the image chunks to Viz as well (Note that this does nothing if
  // sending images is disabled in VizManager)
  robot->GetContext()->GetVizManager()->SendImageChunk(robot->GetID(), payload);
  
  if(isImageReady)
  {
    /* For help debugging COZMO-694:
     PRINT_NAMED_INFO("Robot.HandleImageChunk.ImageReady",
     "About to process image: robot timestamp %d, message time %f, basestation timestamp %d",
     image.GetTimestamp(), message.GetCurrentTime(),
     BaseStationTimer::getInstance()->GetCurrentTimeStamp());
     */
    
    const double currentMessageTime = message.GetCurrentTime();

    if (currentMessageTime != _lastImageRecvTime)
    {
      _lastImageRecvTime = currentMessageTime;
      _repeatedImageCount = 0;
    }
    else
    {
      ++_repeatedImageCount;
      if (ShouldIgnoreMultipleImages())
      {
        PRINT_NAMED_WARNING("RobotImplMessaging.HandleImageChunk",
                            "Ignoring %dth image (with t=%u) received during basestation tick at %fsec",
                            _repeatedImageCount,
                            payload.frameTimeStamp,
                            currentMessageTime);
        
        // Drop the rest of the images on the floor
        return;
      }
    }
    
    //PRINT_NAMED_DEBUG("Robot.HandleImageChunk", "Image at t=%d is ready", _encodedImage.GetTimeStamp());
    
    // NOTE: _encodedImage will be invalidated by this call (SetNextImage does a swap internally).
    //       So don't try to use it for anything else after this!
    robot->GetVisionComponent().SetNextImage(robot->GetEncodedImage());
  } // if(isImageReady)
}
  
bool RobotToEngineImplMessaging::ShouldIgnoreMultipleImages() const
{
  return _repeatedImageCount >= 3;
}

// For processing imu data chunks arriving from robot.
// Writes the entire log of 3-axis accelerometer and 3-axis
// gyro readings to a .m file in kP_IMU_LOGS_DIR so they
// can be read in from Matlab. (See robot/util/imuLogsTool.m)
void RobotToEngineImplMessaging::HandleImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImuData");
  
  const RobotInterface::IMUDataChunk& payload = message.GetData().Get_imuDataChunk();
  
  // If seqID has changed, then start a new log file
  if (payload.seqId != _imuSeqID) {
    _imuSeqID = payload.seqId;
    
    // Make sure imu capture folder exists
    std::string imuLogsDir = robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.HandleImuData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuLog_" + std::to_string(_imuSeqID) + ".dat";
    PRINT_NAMED_INFO("Robot.HandleImuData.OpeningLogFile",
                     "%s", imuLogFileName.c_str());
    
    _imuLogFileStream.open(imuLogFileName.c_str());
    _imuLogFileStream << "aX aY aZ gX gY gZ\n";
  }
  
  for (u32 s = 0; s < (u32)IMUConstants::IMU_CHUNK_SIZE; ++s) {
    _imuLogFileStream << payload.aX.data()[s] << " "
    << payload.aY.data()[s] << " "
    << payload.aZ.data()[s] << " "
    << payload.gX.data()[s] << " "
    << payload.gY.data()[s] << " "
    << payload.gZ.data()[s] << "\n";
  }
  
  // Close file when last chunk received
  if (payload.chunkId == payload.totalNumChunks - 1) {
    PRINT_NAMED_INFO("Robot.HandleImuData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}

void RobotToEngineImplMessaging::HandleImuRawData(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImuRawData");
  
  const RobotInterface::IMURawDataChunk& payload = message.GetData().Get_imuRawDataChunk();
  
  if (payload.order == 0) {
    ++_imuSeqID;
    
    // Make sure imu capture folder exists
    std::string imuLogsDir = robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.HandleImuRawData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuRawLog_" + std::to_string(_imuSeqID) + ".dat";
    PRINT_NAMED_INFO("Robot.HandleImuRawData.OpeningLogFile",
                     "%s", imuLogFileName.c_str());
    
    _imuLogFileStream.open(imuLogFileName.c_str());
    _imuLogFileStream << "timestamp aX aY aZ gX gY gZ\n";
  }
  
  _imuLogFileStream
  << static_cast<int>(payload.timestamp) << " "
  << payload.a.data()[0] << " "
  << payload.a.data()[1] << " "
  << payload.a.data()[2] << " "
  << payload.g.data()[0] << " "
  << payload.g.data()[1] << " "
  << payload.g.data()[2] << "\n";

  // Close file when last chunk received
  if (payload.order == 2) {
    PRINT_NAMED_INFO("Robot.HandleImuRawData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}

void RobotToEngineImplMessaging::HandleImageImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImageImuData");
  
  const ImageImuData& payload = message.GetData().Get_imageGyro();
  
  robot->GetVisionComponent().GetImuDataHistory().AddImuData(payload.imageId,
                                                             payload.rateX,
                                                             payload.rateY,
                                                             payload.rateZ,
                                                             payload.line2Number);
}

void RobotToEngineImplMessaging::HandleSyncTimeAck(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleSyncTimeAck");
  PRINT_NAMED_INFO("Robot.HandleSyncTimeAck","");
  robot->SetTimeSynced();
}

void RobotToEngineImplMessaging::HandleRobotPoked(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotPoked");
  
  // Forward on with EngineToGame event
  PRINT_NAMED_INFO("Robot.HandleRobotPoked","");
  RobotInterface::RobotPoked payload = message.GetData().Get_robotPoked();
  robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPoked(payload.robotID)));
}

void RobotToEngineImplMessaging::HandleDefaultCameraParams(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleDefaultCameraParams");
  
  const DefaultCameraParams& payload = message.GetData().Get_defaultCameraParams();
  robot->GetVisionComponent().HandleDefaultCameraParams(payload);
}

//
// Convert battery voltage to percentage according to profile described by Nathan Monson.
// Always returns a value 0-100.
//
static float GetBatteryPercent(float batteryVoltage)
{
  const float batteryEmpty = 1.0f; // 1.0V
  const float batteryFull = 1.5f; // 1.5V
  
  if (batteryVoltage >= batteryFull) {
    return 100.0f;
  }
  if (batteryVoltage > batteryEmpty) {
    float percent = 100.0f * (batteryVoltage - batteryEmpty) / (batteryFull - batteryEmpty);
    return percent;
  }
  return 0.0f;
}

void RobotToEngineImplMessaging::HandleObjectPowerLevel(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleObjectPowerLevel");
  
  const auto & payload = message.GetData().Get_objectPowerLevel();
  const auto robotID = robot->GetID();
  const auto activeID = payload.objectID;
  const auto missedPackets = payload.missedPackets;
  const auto batteryLevel = payload.batteryLevel;
  const float batteryVoltage = batteryLevel / 100.0f;
  const float batteryPercent = GetBatteryPercent(batteryVoltage);
  
  // Report to log
  PRINT_NAMED_DEBUG("RobotToEngine.ObjectPowerLevel.Log", "RobotID %u activeID %u at %.2fV %.2f%%",
    robotID, activeID, batteryVoltage, batteryPercent);
  
  // Report to DAS if this is first event for this accessory or if appropriate interval has passed since last report
  const uint32_t now = Util::numeric_cast<uint32_t>(Anki::BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());
  const uint32_t then = _lastPowerLevelSentTime[activeID];
  const uint32_t was = _lastMissedPacketCount[activeID];

  if (then == 0 || now - then >= POWER_LEVEL_INTERVAL_SEC || missedPackets - was > 512) {
    PRINT_NAMED_DEBUG("RobotToEngine.ObjectPowerLevel.Report",
                     "Sending DAS report for robotID %u activeID %u now %u then %u",
                     robotID, activeID, now, then);
    char ddata[BUFSIZ];
    snprintf(ddata, sizeof(ddata), "%.2f,%.2f", batteryVoltage, batteryPercent);
    Anki::Util::sEventF("robot.accessory_powerlevel", {{DDATA, ddata}},
      "%u %.2fV (%d lost)", activeID, batteryVoltage, missedPackets);

    _lastPowerLevelSentTime[activeID] = now;
    _lastMissedPacketCount[activeID] = missedPackets;
  }
  
  // Forward to game
  const BlockWorld & blockWorld = robot->GetBlockWorld();
  const ActiveObject * object = blockWorld.GetConnectedActiveObjectByActiveID(activeID);
  if (object != nullptr) {
    const uint32_t objectID = object->GetID();
    PRINT_NAMED_DEBUG("RobotToEngine.ObjectPowerLevel.Broadcast",
                      "RobotID %u activeID %u objectID %u at %u cv",
                      robotID, activeID, objectID, batteryLevel);
    robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectPowerLevel(objectID, missedPackets, batteryLevel)));
  }

}
  
void RobotToEngineImplMessaging::HandleObjectAccel(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleObjectAccel");
  
  const auto& payload = message.GetData().Get_objectAccel();
  const auto activeID = payload.objectID;
  const auto& acc = payload.accel;
  
  //PRINT_NAMED_DEBUG("RobotToEngine.ObjectAccel.Values", "%f %f %f", acc.x, acc.y, acc.z);
  
  robot->GetContext()->GetVizManager()->SendObjectAccelState(activeID, acc);
}
  
} // end namespace Cozmo
} // end namespace Anki
