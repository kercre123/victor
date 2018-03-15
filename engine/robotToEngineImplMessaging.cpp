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

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "util/helpers/printByteArray.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/robotToEngineImplMessaging.h"
#include "engine/actions/actionContainers.h"
#include "engine/actions/animActions.h"
#include "engine/activeObjectHelpers.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/blockTapFilterComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/micDirectionHistory.h"
#include "engine/pathPlanner.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/externalInterface/messageFromActiveObject.h"
#include "clad/externalInterface/messageToActiveObject.h"
#include "clad/types/robotStatusAndActions.h"

#include "audioUtil/audioDataTypes.h"
#include "audioUtil/waveFile.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/debug/messageDebugging.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeFstream.h"
#include "util/signals/signalHolder.h"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include <functional>

#define LOG_CHANNEL "RobotState"

// Prints the IDs of the active blocks that are on but not currently
// talking to a robot whose rssi is less than this threshold.
// Prints roughly once/sec.
#define DISCOVERED_OBJECTS_RSSI_PRINT_THRESH 50

// Filter that makes chargers not discoverable
#define IGNORE_CHARGER_DISCOVERY 0

// How often do we send power level updates to DAS?
#define POWER_LEVEL_INTERVAL_SEC 600

namespace Anki {
namespace Cozmo {
  
using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;

RobotToEngineImplMessaging::RobotToEngineImplMessaging() 
: IDependencyManagedComponent(this, RobotComponentID::RobotToEngineImplMessaging)
, _hasMismatchedEngineToRobotCLAD(false)
, _hasMismatchedRobotToEngineCLAD(false)
{
}

RobotToEngineImplMessaging::~RobotToEngineImplMessaging()
{
}


void RobotToEngineImplMessaging::InitRobotMessageComponent(RobotInterface::MessageHandler* messageHandler, Robot* const robot)
{
  using localHandlerType = void(RobotToEngineImplMessaging::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
  // Create a helper lambda for subscribing to a tag with a local handler
  auto doRobotSubscribe = [this, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(tagType, std::bind(handler, this, std::placeholders::_1)));
  };
  
  using localHandlerTypeWithRoboRef = void(RobotToEngineImplMessaging::*)(const AnkiEvent<RobotInterface::RobotToEngine>&, Robot* const);
  auto doRobotSubscribeWithRoboRef = [this, messageHandler, robot] (RobotInterface::RobotToEngineTag tagType, localHandlerTypeWithRoboRef handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(tagType, std::bind(handler, this, std::placeholders::_1, robot)));
  };
  
  // bind to specific handlers in the robotImplMessaging class
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::factoryFirmwareVersion,         &RobotToEngineImplMessaging::HandleFWVersionInfo);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::pickAndPlaceResult,             &RobotToEngineImplMessaging::HandlePickAndPlaceResult);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::fallingEvent,                   &RobotToEngineImplMessaging::HandleFallingEvent);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::goalPose,                       &RobotToEngineImplMessaging::HandleGoalPose);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotStopped,                   &RobotToEngineImplMessaging::HandleRobotStopped);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::cliffEvent,                     &RobotToEngineImplMessaging::HandleCliffEvent);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::potentialCliff,                 &RobotToEngineImplMessaging::HandlePotentialCliffEvent);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imageGyro,                      &RobotToEngineImplMessaging::HandleImageImuData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imuDataChunk,                   &RobotToEngineImplMessaging::HandleImuData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::imuRawDataChunk,                &RobotToEngineImplMessaging::HandleImuRawData);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::syncTimeAck,                    &RobotToEngineImplMessaging::HandleSyncTimeAck);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotPoked,                     &RobotToEngineImplMessaging::HandleRobotPoked);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::robotAvailable,                 &RobotToEngineImplMessaging::HandleRobotSetHeadID);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::firmwareVersion,                &RobotToEngineImplMessaging::HandleFirmwareVersion);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorCalibration,               &RobotToEngineImplMessaging::HandleMotorCalibration);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::motorAutoEnabled,               &RobotToEngineImplMessaging::HandleMotorAutoEnabled);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::dockingStatus,                             &RobotToEngineImplMessaging::HandleDockingStatus);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::mfgId,                          &RobotToEngineImplMessaging::HandleRobotSetBodyID);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::micDirection,                   &RobotToEngineImplMessaging::HandleMicDirection);
  doRobotSubscribeWithRoboRef(RobotInterface::RobotToEngineTag::streamCameraImages,             &RobotToEngineImplMessaging::HandleStreamCameraImages);  
  
  // lambda wrapper to call internal handler
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::state,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::state");
                                                       const RobotState& payload = message.GetData().Get_state();
                                                       robot->UpdateFullRobotState(payload);
                                                     }));
  
  
  
  // lambda for some simple message handling
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::rampTraverseStarted,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::rampTraverseStarted");
                                                       LOG_INFO("RobotMessageHandler.ProcessMessage",
                                                                "Robot %d reported it started traversing a ramp.",
                                                                robot->GetID());
                                                       robot->SetOnRamp(true);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::rampTraverseCompleted,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::rampTraverseCompleted");
                                                       LOG_INFO("RobotMessageHandler.ProcessMessage",
                                                                "Robot %d reported it completed traversing a ramp.",
                                                                robot->GetID());
                                                       robot->SetOnRamp(false);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::bridgeTraverseStarted,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::bridgeTraverseStarted");
                                                       LOG_INFO("RobotMessageHandler.ProcessMessage",
                                                                "Robot %d reported it started traversing a bridge.",
                                                                robot->GetID());
                                                       //SetOnBridge(true);
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::bridgeTraverseCompleted,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::bridgeTraverseCompleted");
                                                       LOG_INFO("RobotMessageHandler.ProcessMessage",
                                                                "Robot %d reported it completed traversing a bridge.",
                                                                robot->GetID());
                                                       //SetOnBridge(false);
                                                     }));

  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::chargerMountCompleted,
                                                         [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                           ANKI_CPU_PROFILE("RobotTag::chargerMountCompleted");
                                                           const bool didSucceed = message.GetData().Get_chargerMountCompleted().didSucceed;
                                                           LOG_INFO("RobotMessageHandler.ProcessMessage",
                                                                    "Charger mount %s.",
                                                                    didSucceed ? "SUCCEEDED" : "FAILED" );
                                                           if (didSucceed) {
                                                             robot->SetPoseOnCharger();
                                                           }
                                                         }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::imuTemperature,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       ANKI_CPU_PROFILE("RobotTag::imuTemperature");
                                                       
                                                       const auto temp_degC = message.GetData().Get_imuTemperature().temperature_degC;
                                                       // This prints an info every time we receive this message. This is useful for gathering data
                                                       // in the prototype stages, and could probably be removed in production.
                                                       LOG_INFO("RobotMessageHandler.ProcessMessage.MessageImuTemperature",
                                                                "IMU temperature: %.3f degC",
                                                                temp_degC);
                                                       robot->SetImuTemperature(temp_degC);
                                                     }));

  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::enterPairing,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       // Forward to switchboard
                                                       robot->Broadcast(ExternalInterface::MessageEngineToGame(SwitchboardInterface::EnterPairing()));
                                                     }));
  
  GetSignalHandles().push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::exitPairing,
                                                     [robot](const AnkiEvent<RobotInterface::RobotToEngine>& message){
                                                       // Forward to switchboard
                                                       robot->Broadcast(ExternalInterface::MessageEngineToGame(SwitchboardInterface::ExitPairing()));
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
  LOG_INFO("HandleMotorCalibration.Recvd", "Motor %d, started %d, autoStarted %d",
           (int)payload.motorID, payload.calibStarted, payload.autoStarted);
  
  if (payload.calibStarted) {
    Util::sEventF("HandleMotorCalibration.Start",
                  {{DDATA, std::to_string(payload.autoStarted).c_str()}},
                  "%s", EnumToString(payload.motorID));
  } else {
    Util::sEventF("HandleMotorCalibration.Complete",
                  {{DDATA, std::to_string(payload.autoStarted).c_str()}},
                  "%s", EnumToString(payload.motorID));
  }
  
  if (payload.motorID == MotorID::MOTOR_LIFT &&
      payload.calibStarted && robot->GetCarryingComponent().IsCarryingObject())
  {
    // if this was a lift calibration, we are no longer holding a cube
    const bool deleteObjects = true; // we have no idea what happened to the cube, so remove completely from the origin
    robot->GetCarryingComponent().SetCarriedObjectAsUnattached(deleteObjects);
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
  LOG_INFO("HandleMotorAutoEnabled.Recvd", "Motor %d, enabled %d", (int)payload.motorID, payload.enabled);

  if (!payload.enabled) {
    // Burnout protection triggered.
    // Somebody is probably messing with the lift
    LOG_EVENT("HandleMotorAutoEnabled.MotorDisabled", "%s", EnumToString(payload.motorID));
  } else {
    LOG_EVENT("HandleMotorAutoEnabled.MotorEnabled", "%s", EnumToString(payload.motorID));
  }

  // This probably applies here as it does in HandleMotorCalibration.
  // Seems reasonable to expect whatever object the robot may have been carrying to no longer be there.
  if (payload.motorID == MotorID::MOTOR_LIFT &&
      !payload.enabled && robot->GetCarryingComponent().IsCarryingObject()) {
    const bool deleteObjects = true; // we have no idea what happened to the cube, so remove completely from the origin
    robot->GetCarryingComponent().SetCarriedObjectAsUnattached(deleteObjects);
  }
    
  robot->Broadcast(ExternalInterface::MessageEngineToGame(MotorAutoEnabled(payload)));
}

void RobotToEngineImplMessaging::HandleRobotSetHeadID(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotSetHeadID");
  
  const RobotInterface::RobotAvailable& payload = message.GetData().Get_robotAvailable();
  const auto hwRev  = payload.hwRevision;
  const auto headID = payload.serialNumber;
  
  // Set DAS Global on all messages
  char string_id[32] = {};
  snprintf(string_id, sizeof(string_id), "0xbeef%04x%08x", hwRev, headID);
  Anki::Util::sSetGlobal(DGROUP, string_id);
  
  // This should be definition always have a phys ID
  Anki::Util::sEvent("robot.handle_robot_set_head_id", {{DDATA,string_id}}, string_id);
  
  robot->SetHeadSerialNumber(headID);
  robot->SetModelNumber(hwRev);
}
  
void RobotToEngineImplMessaging::HandleRobotSetBodyID(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotSetBodyID");
  
  const RobotInterface::ManufacturingID& payload = message.GetData().Get_mfgId();
  const int32_t hwVersion = payload.hw_version;
  const uint32_t bodyID = payload.esn;
  const int32_t bodyColor = payload.body_color;
  
  // Set DAS Global on all messages
  char string_id[32] = {};
  snprintf(string_id, sizeof(string_id),
           "0xbeef%04x%04x%08x",
           Util::numeric_cast<uint16_t>(bodyColor), // We expect bodyColor and hwVersion to always be +ve
           Util::numeric_cast<uint16_t>(hwVersion),
           bodyID);
  
  Anki::Util::sSetGlobal(DPHYS, string_id);
  Anki::Util::sEvent("robot.handle_robot_set_body_id", {{DDATA,string_id}}, string_id);
  
  robot->SetBodySerialNumber(bodyID);
  robot->SetBodyHWVersion(hwVersion);
  robot->SetBodyColor(bodyColor);

  // Activate A/B tests for robot now that we have its serial
  robot->GetContext()->GetExperiments()->AutoActivateExperiments(std::to_string(bodyID));
}
  
void RobotToEngineImplMessaging::HandleFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  // Extract sim flag from json
  const auto& fwData = message.GetData().Get_firmwareVersion().json;
  std::string jsonString{fwData.begin(), fwData.end()};
  Json::Reader reader;
  Json::Value headerData;
  if (!reader.parse(jsonString, headerData))
  {
    return;
  }
  
  // simulated robot will have special tag in json
  const bool robotIsPhysical = headerData["sim"].isNull();

  LOG_INFO("RobotIsPhysical", "%d", robotIsPhysical);
  robot->SetPhysicalRobot(robotIsPhysical);
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
    
    LOG_WARNING("RobotFirmware.VersionMismatch",
                "Engine to Robot CLAD version hash mismatch. Robot's EngineToRobot hash = %s. Engine's EngineToRobot hash = %s.",
                robotEngineToRobotStr.c_str(), engineEngineToRobotStr.c_str());
    
    _hasMismatchedEngineToRobotCLAD = true;
  }
  
  std::string robotRobotToEngineStr;
  std::string engineRobotToEngineStr;
  
  if (memcmp(_factoryFirmwareVersion.toEngineCLADHash.data(), messageRobotToEngineHash, _factoryFirmwareVersion.toEngineCLADHash.size())) {

    robotRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(_factoryFirmwareVersion.toEngineCLADHash.data(), static_cast<uint32_t>(_factoryFirmwareVersion.toEngineCLADHash.size()), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    engineRobotToEngineStr = Anki::Util::ConvertMessageBufferToString(messageRobotToEngineHash, sizeof(messageRobotToEngineHash), Anki::Util::EBytesToTextType::eBTTT_Hex);
    
    LOG_WARNING("RobotFirmware.VersionMismatch",
                "Robot to Engine CLAD version hash mismatch. Robot's RobotToEngine hash = %s. Engine's RobotToEngine hash = %s.",
                robotRobotToEngineStr.c_str(), engineRobotToEngineStr.c_str());
    
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
  
  robot->GetDockingComponent().SetLastPickOrPlaceSucceeded(payload.didSucceed);
  
  switch(payload.blockStatus)
  {
    case BlockStatus::NO_BLOCK:
    {
      LOG_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.NoBlock",
               "Robot %d reported it %s doing something without a block. Stopping docking and turning on Look-for-Markers mode.",
               robot->GetID(), successStr);
      break;
    }
    case BlockStatus::BLOCK_PLACED:
    {
      LOG_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.BlockPlaced",
               "Robot %d reported it %s placing block. Stopping docking and turning on Look-for-Markers mode.",
               robot->GetID(), successStr);
    
      if (payload.didSucceed) {
        robot->GetCarryingComponent().SetCarriedObjectAsUnattached();
      }
      
      robot->GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
      
      break;
    }
    case BlockStatus::BLOCK_PICKED_UP:
    {
      const char* resultStr = EnumToString(payload.result);
      
      LOG_INFO("RobotMessageHandler.ProcessMessage.HandlePickAndPlaceResult.BlockPickedUp",
               "Robot %d reported it %s picking up block with %s. Stopping docking and turning on Look-for-Markers mode.",
               robot->GetID(), successStr, resultStr);
    
      if (payload.didSucceed) {
        robot->GetCarryingComponent().SetDockObjectAsAttachedToLift();
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
  if (nullptr == connectedObj) {
    LOG_WARNING(MAKE_EVENT_NAME("UnknownActiveID"), "Could not find match for active object ID %d", payload.objectID);
  }
  else
  {
    // Only do this stuff once, since these checks should be the same across all frames. Use connected instance
    if (connectedObj->GetID() == robot->GetCharger())
    {
      LOG_INFO(MAKE_EVENT_NAME("Charger"), "Charger sending garbage move messages");
      return;
    }
  
    DEV_ASSERT(connectedObj->IsActive(), MAKE_EVENT_NAME("NonActiveObject"));
  
    LOG_INFO(MAKE_EVENT_NAME("ObjectMovedOrStopped"),
             "ObjectID: %d (Active ID %d), type: %s, axisOfAccel: %s, accel: %f %f %f, time: %d ms",
             connectedObj->GetID().GetValue(), connectedObj->GetActiveID(),
             EnumToString(connectedObj->GetType()), GetAxisString(payload),
             GetXAccelVal(payload), GetYAccelVal(payload), GetZAccelVal(payload), payload.timestamp );
    
    const bool shouldIgnoreMovement = robot->GetBlockTapFilter().ShouldIgnoreMovementDueToDoubleTap(connectedObj->GetID());
    if (shouldIgnoreMovement && GetIsMoving<PayloadType>())
    {
      LOG_INFO(MAKE_EVENT_NAME("IgnoringMessage"),
               "Waiting for double tap id:%d ignoring movement message",
               connectedObj->GetID().GetValue());
      return;
    }
    
    // for later notification
    matchedObjectID = connectedObj->GetID();
    
    // update Moving flag of connected object when it changes
    const bool wasMoving = connectedObj->IsMoving();
    const bool isMovingNow = GetIsMoving<PayloadType>();
    if (wasMoving != isMovingNow) {
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
  
  for (ObservableObject* object : matchingObjects)
  {
    assert(object != nullptr); // FindMatchingObjects should not return nullptrs
    
    if (object->GetID() != matchedObjectID)
    {
      LOG_WARNING(MAKE_EVENT_NAME("ActiveObjectInDifferentFramesWithDifferentIDs"),
                  "First object=%d in '%s'. This object=%d in '%s'.",
                  matchingObjects.front()->GetID().GetValue(),
                  matchingObjects.front()->GetPose().FindRoot().GetName().c_str(),
                  object->GetID().GetValue(),
                  object->GetPose().FindRoot().GetName().c_str());
    }
    
    // We expect carried objects to move, so don't mark them as dirty/inaccurate.
    // Their pose state should remain accurate/known because they are attached to
    // the lift. I'm leaving this a separate check from the decision about broadcasting
    // the movement, in case we want to easily remove the checks above but keep this one.
    const bool isCarryingObject = robot->GetCarryingComponent().IsCarryingObject(object->GetID());
    if (object->IsPoseStateKnown() && !isCarryingObject)
    {
      // Once an object moves, we can no longer use it for localization because
      // we don't know where it is anymore. Next time we see it, relocalize it
      // relative to robot's pose estimate. Then we can use it for localization
      // again.
      const bool propagateStack = false;
      robot->GetObjectPoseConfirmer().MarkObjectDirty(object, propagateStack);
    }
    
    const bool wasMoving = object->IsMoving();
    const bool isMovingNow = GetIsMoving<PayloadType>();
    if (wasMoving != isMovingNow)
    {
      // Set moving state of object (in any frame)
      object->SetIsMoving(GetIsMoving<PayloadType>(), payload.timestamp);
    }
    
  } // for(ObservableObject* object : matchingObjects)
  
  if (matchedObjectID.IsSet())
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
    const bool isDockingObject = (connectedObj->GetID() == robot->GetDockingComponent().GetDockObject());
    const bool isCarryingObject = robot->GetCarryingComponent().IsCarryingObject(connectedObj->GetID());
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = matchedObjectID;
    
    if (!isDockingObject && !isCarryingObject)
    {
      robot->Broadcast(ExternalInterface::MessageEngineToGame(PayloadType(payload)));
    }
  }
  else
  {
    LOG_WARNING("ObjectMovedOrStoppedHelper.UnknownActiveID", "Could not find match for active object ID %d", activeID);
  }
  
# undef MAKE_EVENT_NAME
  
} // ObjectMovedOrStoppedHelper()

void RobotToEngineImplMessaging::HandleActiveObjectMoved(const ObjectMoved& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectMoved");
  
  ObjectMovedOrStoppedHelper(robot, message);
}

void RobotToEngineImplMessaging::HandleActiveObjectStopped(const ObjectStoppedMoving& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectStopped");
  
  ObjectMovedOrStoppedHelper(robot, message);
}


void RobotToEngineImplMessaging::HandleActiveObjectUpAxisChanged(const ObjectUpAxisChanged& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleActiveObjectUpAxisChanged");

  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectUpAxisChanged payload = message;
  
  
  // grab objectID from the connected instance
  ActiveID activeID = payload.objectID;
  const ObservableObject* conObj = robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(activeID);
  if (nullptr == conObj)
  {
    LOG_ERROR("Robot.HandleActiveObjectUpAxisChanged.UnknownActiveID",
              "Could not find match for active object ID %d",
              payload.objectID);
    return;
  }

  LOG_INFO("Robot.HandleActiveObjectUpAxisChanged.UpAxisChanged",
           "Type: %s, ObjectID: %d, UpAxis: %s",
           EnumToString(conObj->GetType()),
           conObj->GetID().GetValue(),
           EnumToString(payload.upAxis));
  
  // Viz update
  robot->GetContext()->GetVizManager()->SendObjectUpAxisState(payload.objectID, payload.upAxis);
  
  // Update the ID to be the blockworld ID before broadcasting
  payload.objectID = conObj->GetID();
  robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(payload)));
}

void RobotToEngineImplMessaging::HandleFallingEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  const auto& msg = message.GetData().Get_fallingEvent();
  
  LOG_INFO("Robot.HandleFallingEvent.FallingEvent",
           "timestamp: %u, duration (ms): %u, intensity %.1f",
           msg.timestamp,
           msg.duration_ms,
           msg.impactIntensity);
  
  // DAS Event: "robot.falling_event"
  // s_val: Impact intensity
  // data: Freefall duration in milliseconds
  const int impactIntensity_int = std::round(msg.impactIntensity);
  Util::sEvent("robot.falling_event",                              // 'event'
               {{DDATA, std::to_string(msg.duration_ms).c_str()}}, // 'data'
               std::to_string(impactIntensity_int).c_str());       // 's_val'
  
  // TODO: Beam this up to game?
  // robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(payload)));
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
                {{DDATA, ""}},
                "%d", payload.reason);
  
  // This is a somewhat overloaded use of enableCliffSensor, but currently only cliffs
  // trigger this RobotStopped message so it's not too crazy.
  if( !(robot->GetCliffSensorComponent().IsCliffSensorEnabled()) ) {
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
  
  // Ignore potential cliff events while on the charger platform because we expect them
  // while driving off the charger
  if (robot->IsOnChargerPlatform())
  {
    LOG_DEBUG("Robot.HandlePotentialCliffEvent.OnChargerPlatform",
              "Ignoring potential cliff event while on charger platform");
    return;
  }
  
  if (robot->GetIsCliffReactionDisabled()){
    // Special case handling of potential cliff event when in drone/explorer mode...

    // TODO: Don't try to play this special cliff event animation for drone/explorer mode if it is already
    //       running. Consider adding support for a 'canBeInterrupted' flag or something similar and then
    //       set canBeInterrupted = false before queueing this action to run now (VIC-796). FYI, a different
    //       solution was used for Cozmo (see COZMO-15326 and https://github.com/anki/cozmo-one/pull/6467)

    // Trigger the cliff event animation for drone/explorer mode if it is not already running and:
    // - set interruptRunning = true so any currently-streaming animation will be aborted in favor of this
    // - set a timeout value of 3 seconds for this animation
    // - set strictCooldown = true so we do NOT simply choose the animation closest to being off
    //   cooldown when all animations in the group are on cooldown
    IActionRunner* action = new TriggerLiftSafeAnimationAction(AnimationTrigger::AudioOnlyHuh, 1,
                                                               true, (u8)AnimTrackFlag::NO_TRACKS, 3.f, true);
    robot->GetActionList().QueueAction(QueueActionPosition::NOW, action);
  } else if (!robot->GetContext()->IsInSdkMode()) {
    LOG_WARNING("Robot.HandlePotentialCliffEvent", "Got potential cliff message but not in drone mode");
    robot->GetMoveComponent().StopAllMotors();
    robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  }
}

void RobotToEngineImplMessaging::HandleCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleCliffEvent");
  
  CliffEvent cliffEvent = message.GetData().Get_cliffEvent();
  // always listen to events which say we aren't on a cliff, but ignore ones which say we are (so we don't
  // get "stuck" on a cliff
  if (!robot->GetCliffSensorComponent().IsCliffSensorEnabled() && (cliffEvent.detectedFlags != 0)) {
    return;
  }
  
  if (cliffEvent.detectedFlags != 0) {
    Pose3d cliffPose;
    if (robot->GetCliffSensorComponent().ComputeCliffPose(cliffEvent, cliffPose)) {
      // Add cliff obstacle
      robot->GetBlockWorld().AddCliff(cliffPose);
      LOG_INFO("RobotImplMessaging.HandleCliffEvent.Detected", "at %.3f,%.3f. DetectedFlags = 0x%02X",
               cliffPose.GetTranslation().x(), cliffPose.GetTranslation().y(), cliffEvent.detectedFlags);
    } else {
      LOG_ERROR("RobotImplMessaging.HandleCliffEvent.ComputeCliffPoseFailed",
                "Failed computing cliff pose!");
    }
  } else {
    LOG_INFO("RobotImplMessaging.HandleCliffEvent.Undetected", "");
  }
  
  robot->GetCliffSensorComponent().SetCliffDetectedFlags(cliffEvent.detectedFlags);
  
  // Forward on with EngineToGame event
  robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(cliffEvent)));
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
      LOG_ERROR("Robot.HandleImuData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuLog_" + std::to_string(_imuSeqID) + ".dat";

    LOG_INFO("Robot.HandleImuData.OpeningLogFile", "%s", imuLogFileName.c_str());
    
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
    LOG_INFO("Robot.HandleImuData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}

void RobotToEngineImplMessaging::HandleImuRawData(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImuRawData");
  
  const RobotInterface::IMURawDataChunk& payload = message.GetData().Get_imuRawDataChunk();
  
  if (payload.order == 0) {
    
    // Make sure imu capture folder exists
    std::string imuLogsDir = robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      LOG_ERROR("Robot.HandleImuRawData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = "";
    do {
      ++_imuSeqID;
      imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuRawLog_" + std::to_string(_imuSeqID) + ".dat";
    } while (Util::FileUtils::FileExists(imuLogFileName));
    
    LOG_INFO("Robot.HandleImuRawData.OpeningLogFile",
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
    LOG_INFO("Robot.HandleImuRawData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}

void RobotToEngineImplMessaging::HandleImageImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleImageImuData");
  
  const ImageImuData& payload = message.GetData().Get_imageGyro();
  
  robot->GetVisionComponent().GetImuDataHistory().AddImuData(payload.systemTimestamp_ms,
                                                             payload.rateX,
                                                             payload.rateY,
                                                             payload.rateZ);
}

void RobotToEngineImplMessaging::HandleSyncTimeAck(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleSyncTimeAck");
  LOG_INFO("Robot.HandleSyncTimeAck","");
  robot->SetTimeSynced();

  // Move the head up when we sync time so that the customer can see the face easily
  if(FACTORY_TEST && Factory::GetEMR()->PACKED_OUT_FLAG)
  {
    const f32 kLookUpSpeed_radps = 2;
    robot->GetMoveComponent().MoveHeadToAngle(MAX_HEAD_ANGLE, 
                                              kLookUpSpeed_radps, 
                                              MAX_HEAD_ACCEL_RAD_PER_S2);
  }
}

void RobotToEngineImplMessaging::HandleRobotPoked(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleRobotPoked");
  
  // Forward on with EngineToGame event
  LOG_INFO("Robot.HandleRobotPoked","");
  robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPoked()));
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

void RobotToEngineImplMessaging::HandleObjectPowerLevel(const ObjectPowerLevel& payload, Robot* const robot)
{
  ANKI_CPU_PROFILE("Robot::HandleObjectPowerLevel");
  
  const auto robotID __attribute__((unused)) = robot->GetID();
  const auto activeID = payload.objectID;
  const auto missedPackets = payload.missedPackets;
  const auto batteryLevel = payload.batteryLevel;
  const float batteryVoltage = batteryLevel / 100.0f;
  const float batteryPercent = GetBatteryPercent(batteryVoltage);
  
  // Report to log
  LOG_DEBUG("RobotToEngine.ObjectPowerLevel.Log", "RobotID %u activeID %u at %.2fV %.2f%%",
            robotID, activeID, batteryVoltage, batteryPercent);
  
  // Report to DAS if this is first event for this accessory or if appropriate interval has passed since last report
  const uint32_t now = Util::numeric_cast<uint32_t>(Anki::BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());
  const uint32_t then = _lastPowerLevelSentTime[activeID];
  const uint32_t was = _lastMissedPacketCount[activeID];

  if (then == 0 || now - then >= POWER_LEVEL_INTERVAL_SEC || missedPackets - was > 512) {
    LOG_DEBUG("RobotToEngine.ObjectPowerLevel.Report",
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
    LOG_DEBUG("RobotToEngine.ObjectPowerLevel.Broadcast",
              "RobotID %u activeID %u objectID %u at %u cv",
              robotID, activeID, objectID, batteryLevel);
    robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectPowerLevel(objectID, missedPackets, batteryLevel)));
  }

}

void RobotToEngineImplMessaging::HandleMicDirection(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  const auto & payload = message.GetData().Get_micDirection();
  robot->GetMicDirectionHistory().AddDirectionSample(payload.timestamp, payload.direction, payload.confidence);
}

void RobotToEngineImplMessaging::HandleStreamCameraImages(const AnkiEvent<RobotInterface::RobotToEngine>& message, Robot* const robot)
{
  const auto & payload = message.GetData().Get_streamCameraImages();
  robot->GetVisionComponent().EnableDrawImagesToScreen(payload.enable);
}

} // end namespace Cozmo
} // end namespace Anki
