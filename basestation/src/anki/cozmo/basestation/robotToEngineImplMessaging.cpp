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

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/RobotToEngineImplMessaging.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/externalInterface/messageEngineToGame.h"
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
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotAvailable,                 &RobotToEngineImplMessaging::HandleRobotSetID);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::firmwareVersion,                 &RobotToEngineImplMessaging::HandleFirmwareVersion);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorCalibration,               &RobotToEngineImplMessaging::HandleMotorCalibration);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorAutoEnabled,               &RobotToEngineImplMessaging::HandleMotorAutoEnabled);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::dockingStatus,                             &RobotToEngineImplMessaging::HandleDockingStatus);
  
  
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
  }
}

void RobotToEngineImplMessaging::HandleMotorCalibration(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleMotorCalibration");
  
  const MotorCalibration& payload = message.GetData().Get_motorCalibration();
  PRINT_NAMED_INFO("HandleMotorCalibration.Recvd", "Motor %d, started %d, autoStarted %d", (int)payload.motorID, payload.calibStarted, payload.autoStarted);
  
  if (payload.autoStarted && payload.calibStarted) {
    // Motor hit a limit and calibration was automatically triggered
    PRINT_NAMED_EVENT("HandleMotorCalibration.AutoCalib", "%s", EnumToString(payload.motorID));
  }
  
  if( payload.motorID == MotorID::MOTOR_LIFT && payload.calibStarted && robot->IsCarryingObject() ) {
    // if this was a lift calibration, we are no longer holding a cube
    robot->UnSetCarryObject( robot->GetCarryingObject() );
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
    PRINT_NAMED_EVENT("HandleMotorAutoEnabled.MotorDisabled", "%s", EnumToString(payload.motorID));
  } else {
    PRINT_NAMED_EVENT("HandleMotorAutoEnabled.MotorEnabled", "%s", EnumToString(payload.motorID));
  }

  // This probably applies here as it does in HandleMotorCalibration.
  // Seems reasonable to expect whatever object the robot may have been carrying to no longer be there.
  if( payload.motorID == MotorID::MOTOR_LIFT && !payload.enabled && robot->IsCarryingObject() ) {
    robot->UnSetCarryObject( robot->GetCarryingObject() );
  }
    
  robot->Broadcast(ExternalInterface::MessageEngineToGame(MotorAutoEnabled(payload)));
}

void RobotToEngineImplMessaging::HandleRobotSetID(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotSetID");
  
  const RobotInterface::RobotAvailable& payload = message.GetData().Get_robotAvailable();
  // Set DAS Global on all messages
  char string_id[32] = {0};
  snprintf(string_id, sizeof(string_id), "0xbeef%04x%08x", payload.modelID,payload.robotID);
  Anki::Util::sSetGlobal(DPHYS, string_id);
  
  // This should be definition always have a phys ID
  Anki::Util::sEvent("robot.handleRobotSetID",{},string_id);
  
  robot->SetSerialNumber(payload.robotID);
  robot->SetModelNumber(payload.modelID);
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
    PRINT_NAMED_ERROR("RobotToEngineImpleMessaging.HandleFirmwareVersion.ParseJson",
                      "Failed to parse header data from robot: %s", jsonString.c_str());
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
    
    PRINT_NAMED_WARNING("RobotFirmware.VersionMissmatch", "Engine to Robot CLAD version hash mismatch. Robot's EngineToRobot hash = %s. Engine's EngineToRobot hash = %s.", robotEngineToRobotStr.c_str(), engineEngineToRobotStr.c_str());
    
    _hasMismatchedEngineToRobotCLAD = true;
  }
  
  std::string robotRobotToEngineStr;
  std::string engineRobotToEngineStr;
  
  if (memcmp(_factoryFirmwareVersion.toEngineCLADHash.data(), messageRobotToEngineHash, _factoryFirmwareVersion.toEngineCLADHash.size())) {

    robotRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(_factoryFirmwareVersion.toEngineCLADHash.data(), static_cast<uint32_t>(_factoryFirmwareVersion.toEngineCLADHash.size()), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    engineRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(messageRobotToEngineHash, sizeof(messageRobotToEngineHash), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    PRINT_NAMED_WARNING("RobotFirmware.VersionMissmatch", "Robot to Engine CLAD version hash mismatch. Robot's RobotToEngine hash = %s. Engine's RobotToEngine hash = %s.", robotRobotToEngineStr.c_str(), engineRobotToEngineStr.c_str());
    
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
  PRINT_NAMED_EVENT("robot.docking.status", "%s", EnumToString(message.GetData().Get_dockingStatus().status));
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
    ASSERT_NAMED(false, "Robot.HandleActiveObjectConnectionState.InvalidActiveID");
    return;
  }
  
  
  if (payload.connected) {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"connected"}}, "0x%x,%s", payload.factoryID, EnumToString(payload.device_type));

    // Add active object to blockworld if not already there
    objID = robot->GetBlockWorld().AddActiveObject(payload.objectID, payload.factoryID, payload.device_type);
    if (objID.IsSet()) {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Connected",
                       "Object %d (activeID %d, factoryID 0x%x, device_type 0x%hx)",
                       objID.GetValue(), payload.objectID, payload.factoryID, payload.device_type);
      
      // if a charger, and robot is on the charger, add a pose for the charager
      if( payload.device_type == Anki::Cozmo::ActiveObjectType::OBJECT_CHARGER )
      {
        robot->SetCharger(objID);
        if( robot->IsOnCharger() )
        {
          Charger* charger = dynamic_cast<Charger*>(robot->GetBlockWorld().GetObjectByID(objID, ObjectFamily::Charger));
          if( nullptr != charger )
          {
            charger->SetPoseRelativeToRobot(*robot);
          }
        }
      }
      
      ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(payload.device_type);
      robot->HandleConnectedToObject(payload.objectID, payload.factoryID, objType);
    }
  } else {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"disconnected"}}, "0x%x,%s", payload.factoryID, EnumToString(payload.device_type));

    // Remove active object from blockworld if it exists (in any coordinate frame)
    BlockWorldFilter filter;
    filter.SetFilterFcn([&payload](const ObservableObject* object) {
      return object->IsActive() && object->GetActiveID() == payload.objectID;
    });
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    
    std::vector<ObservableObject*> matchingObjects;
    robot->GetBlockWorld().FindMatchingObjects(filter, matchingObjects);
    
    if(matchingObjects.empty()) {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.SlotAlreadyDisconnected",
                       "Received disconnected for activeID %d, factoryID 0x%x, but slot is already disconnected",
                       payload.objectID, payload.factoryID);
    } else {
      for(auto obj : matchingObjects)
      {
        bool clearedObject = false;
        if(objID.IsUnknown()) {
          objID = obj->GetID();
        } else {
          // We expect all objects with the same active ID (across coordinate frames) to have the same
          // object ID
          ASSERT_NAMED_EVENT(objID == obj->GetID(), "Robot.HandleActiveObjectConnectionState.MismatchedIDs",
                             "Object %d (activeID %d, factoryID 0x%x, device_type 0x%hx, origin %p)",
                             objID.GetValue(), payload.objectID, payload.factoryID,
                             payload.device_type, &obj->GetPose().FindOrigin());
          
          // When disconnecting from an object make sure to set the factoryIDs of all matching objects in all frames
          // to zero in case we see this disconnected object in the world
          // Also update activeIDs so we don't try to localize to it
          obj->SetFactoryID(0);
          obj->SetActiveID(-1);
        }
        
        if( ! obj->IsPoseStateKnown() ) {
          robot->GetBlockWorld().ClearObject(objID);
          clearedObject = true;
        }
        
        PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Disconnected",
                         "Object %d (activeID %d, factoryID 0x%x, device_type 0x%hx, origin %p) cleared? %d",
                         objID.GetValue(), payload.objectID, payload.factoryID,
                         payload.device_type, &obj->GetPose().FindOrigin(), clearedObject);
      } // for(auto obj : matchingObjects)
    }
    
    ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(payload.device_type);
    robot->HandleDisconnectedFromObject(payload.objectID, payload.factoryID, objType);
  }
  
  PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Recvd",
                   "FactoryID 0x%x, connected %d",
                   payload.factoryID, payload.connected);
  
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
  
template<class PayloadType> static bool GetIsMoving();
template<> inline bool GetIsMoving<ObjectMoved>() { return true; }
template<> inline bool GetIsMoving<ObjectStoppedMoving>() { return false; }

  
// Shared helper we can use for Moved or Stopped messages
template<class PayloadType>
static void ObjectMovedOrStoppedHelper(Robot* const robot, PayloadType payload)
{
  const std::string eventPrefix = GetEventPrefix<PayloadType>();
  
# define MAKE_EVENT_NAME(__str__) (eventPrefix + __str__).c_str()
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID. We also need to consider all pose states and origin frames.
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn([&payload](const ObservableObject* object) {
    return object->IsActive() && object->GetActiveID() == payload.objectID;
  });
  
  std::vector<ObservableObject *> matchingObjects;
  robot->GetBlockWorld().FindMatchingObjects(filter, matchingObjects);
  
  if(matchingObjects.empty())
  {
    PRINT_NAMED_WARNING(MAKE_EVENT_NAME("UnknownActiveID"),
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  
  // Only do this stuff once, since these checks should be the same across all frames:
  const ObservableObject* firstObject = matchingObjects.front();
  assert(firstObject != nullptr); // FindMatchingObjects should not return nullptrs
  if( firstObject->GetID() == robot->GetCharger() )
  {
    PRINT_NAMED_INFO(MAKE_EVENT_NAME("Charger"), "Charger sending garbage move messages");
    return;
  }
  
  ASSERT_NAMED(firstObject->IsActive(), MAKE_EVENT_NAME("NonActiveObject"));
  
  PRINT_NAMED_INFO(MAKE_EVENT_NAME("ObjectMovedOrStopped"),
                   "ObjectID: %d (Active ID %d), type: %s, axisOfAccel: %s",
                   firstObject->GetID().GetValue(), firstObject->GetActiveID(),
                   EnumToString(firstObject->GetType()), GetAxisString(payload));
  
  // Don't notify game about moving objects that are being carried or docked with
  // (We expect those to move)
  const bool isDockingObject  = firstObject->GetID() == robot->GetDockObject();
  const bool isCarryingObject = robot->IsCarryingObject(firstObject->GetID());
  
  if(!isCarryingObject && !isDockingObject)
  {
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = firstObject->GetID();
    payload.robotID = robot->GetID();
    robot->Broadcast(ExternalInterface::MessageEngineToGame(PayloadType(payload)));
  }

  for(ObservableObject* object : matchingObjects)
  {
    assert(object != nullptr); // FindMatchingObjects should not return nullptrs
    
    if(object->GetID() != matchingObjects.front()->GetID())
    {
      PRINT_NAMED_WARNING(MAKE_EVENT_NAME("ActiveObjectInDifferentFramesWithDifferentIDs"),
                          "First object=%d in '%s'. This object=%d in '%s'.",
                          matchingObjects.front()->GetID().GetValue(),
                          matchingObjects.front()->GetPose().FindOrigin().GetName().c_str(),
                          object->GetID().GetValue(),
                          object->GetPose().FindOrigin().GetName().c_str());
    }
    
    const bool isInCurrentFrame = &object->GetPose().FindOrigin() == robot->GetWorldOrigin();
    
    if(isInCurrentFrame)
    {
      if(object->IsPoseStateKnown())
      {
        PRINT_NAMED_INFO(MAKE_EVENT_NAME("DelocalizingObject"),
                         "ObjectID: %d (Active ID %d), type: %s",
                         object->GetID().GetValue(), object->GetActiveID(),
                         EnumToString(object->GetType()));
        
        // Once an object moves, we can no longer use it for localization because
        // we don't know where it is anymore. Next time we see it, relocalize it
        // relative to robot's pose estimate. Then we can use it for localization
        // again.
        object->SetPoseState(PoseState::Dirty);
        
        // If this is the object we were localized to, unset our localizedToID.
        // Note we are still "localized" by odometry, however.
        if(robot->GetLocalizedTo() == object->GetID())
        {
          ASSERT_NAMED(robot->IsLocalized(), MAKE_EVENT_NAME("BadIsLocalizedCheck"));
          PRINT_NAMED_INFO(MAKE_EVENT_NAME("UnsetLocalzedToID"),
                           "Unsetting %s %d, which moved/stopped, as robot %d's localization object.",
                           ObjectTypeToString(object->GetType()), object->GetID().GetValue(), robot->GetID());
          robot->SetLocalizedTo(nullptr);
        }
        else if(!(robot->IsLocalized()) && robot->GetOffTreadsState() == OffTreadsState::OnTreads)
        {
          // If we are not localized and there is nothing else left in the world that
          // we could localize to, then go ahead and mark us as localized (via
          // odometry alone)
          if(false == robot->GetBlockWorld().AnyRemainingLocalizableObjects()) {
            PRINT_NAMED_INFO(MAKE_EVENT_NAME("NoMoreRemainingLocalizableObjects"),
                             "Marking previously-unlocalized robot %d as localized to odometry because "
                             "there are no more objects to localize to in the world.", robot->GetID());
            robot->SetLocalizedTo(nullptr);
          }
        }
      }
    }
    else
    {
      // For objects in other frames, we just need to make sure to mark dirty
      // if they were previously known
      if(object->IsPoseStateKnown())
      {
        object->SetPoseState(PoseState::Dirty);
      }
    }
    
    // Set moving state of object (in any frame)
    object->SetIsMoving(GetIsMoving<PayloadType>(), payload.timestamp);
    
  } // for(ObservableObject* object : matchingObjects)
                           
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
  robot->GetBlockWorld().FindMatchingObjects(filter, matchingObjects);

  if(matchingObjects.empty())
  {
    PRINT_NAMED_WARNING("Robot.HandleActiveObjectUpAxisChanged.UnknownActiveID",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }

  // Just use the first matching object since they will all be the same (matching ObjectIDs, ActiveIDs, FactoryIDs)
  ObservableObject* object = matchingObjects.front();
  
  PRINT_NAMED_INFO("Robot.HandleActiveObjectUpAxisChanged.UpAxisChanged",
                   "Type: %s, ObjectID: %d, UpAxis: %s",
                   EnumToString(object->GetType()),
                   object->GetID().GetValue(),
                   EnumToString(payload.upAxis));
  
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
  //PRINT_INFO("Goal pose: x=%f y=%f %f deg (%d)", msg.pose_x, msg.pose_y, RAD_TO_DEG_F32(msg.pose_angle), msg.followingMarkerNormal);
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
  PRINT_NAMED_INFO("RobotImplMessaging.HandleRobotStopped", "%d", payload.reason);
  
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
    robot->GetActionList().QueueActionNow(action);
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
  if (robot->GetContext()->GetExternalInterface() != nullptr && robot->GetImageSendMode() != ImageSendMode::Off)
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
      if (_repeatedImageCount >= 3)
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
  << payload.a.data()[0] << " "
  << payload.a.data()[1] << " "
  << payload.a.data()[2] << " "
  << payload.g.data()[0] << " "
  << payload.g.data()[1] << " "
  << payload.g.data()[2] << " "
  << static_cast<int>(payload.timestamp) << "\n";
  
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
  
  robot->SetTimeSynced(true);
}

void RobotToEngineImplMessaging::HandleRobotPoked(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotPoked");
  
  // Forward on with EngineToGame event
  PRINT_NAMED_INFO("Robot.HandleRobotPoked","");
  RobotInterface::RobotPoked payload = message.GetData().Get_robotPoked();
  robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPoked(payload.robotID)));
}
} // end namespace Cozmo
} // end namespace Anki
