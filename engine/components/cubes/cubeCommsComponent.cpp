/**
 * File: cubeCommsComponent.cpp
 *
 * Author: Matt Michini
 * Created: 11/29/2017
 *
 * Description: Component for managing communications with light cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeCommsComponent.h"

#include "engine/components/blockTapFilterComponent.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/cubes/ledAnimation.h"
#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/robotToEngineImplMessaging.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/jsonTools.h"

#include "clad/externalInterface/messageCubeToEngine.h"
#include "clad/externalInterface/messageEngineToCube.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "util/fileUtils/fileUtils.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/console/consoleInterface.h"

#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

namespace {
  // This is the only cube type that we expect to communicate with
  const ObjectType kValidCubeType = ObjectType::Block_LIGHTCUBE1;
  
  // How long to remain in discovery mode
#ifdef SIMULATOR
  const float kDefaultDiscoveryTime_sec = 3.f;
#else
  const float kDefaultDiscoveryTime_sec = 7.f;
#endif

  const float kSendWebVizDataPeriod_sec = 1.0f;
  
  const int kNumCubeLEDs = Util::EnumToUnderlying(CubeConstants::NUM_CUBE_LEDS);
  
  const std::string kBlockFacIdKey = "blockFactoryId";
  
  const std::string kCubeFirmwarePath = "assets/cubeFirmware/cube.dfu";
  
  const std::string kPreferredCubeFilename = "preferredCube.json";

  const std::string kWebVizModuleNameCubes = "cubes";
}

CubeCommsComponent::CubeCommsComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeComms)
, _cubeBleClient(std::make_unique<CubeBleClient>())
{
  // Register callbacks for messages from CubeBleClient
  _cubeBleClient->RegisterObjectAvailableCallback  (std::bind(&CubeCommsComponent::HandleObjectAvailable,       this, std::placeholders::_1));
  _cubeBleClient->RegisterCubeMessageCallback      (std::bind(&CubeCommsComponent::HandleCubeMessage,           this, std::placeholders::_1, std::placeholders::_2));
  _cubeBleClient->RegisterCubeConnectionCallback   (std::bind(&CubeCommsComponent::HandleConnectionStateChange, this, std::placeholders::_1, std::placeholders::_2));
  _cubeBleClient->RegisterScanFinishedCallback     (std::bind(&CubeCommsComponent::HandleScanForCubesFinished,  this));
  _cubeBleClient->RegisterConnectionFailedCallback (std::bind(&CubeCommsComponent::HandleConnectionFailed,      this, std::placeholders::_1));
}


void CubeCommsComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  
  // Game to engine message handling:
  if (ANKI_VERIFY(_robot->HasExternalInterface(),
                  "CubeCommsComponent.InitDependent.NoExternalInterface", "")) {
    auto callback = std::bind(&CubeCommsComponent::HandleGameEvents, this, std::placeholders::_1);
    auto* extInterface = _robot->GetExternalInterface();
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToCube, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromCube, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ForgetPreferredCube, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::SetPreferredCube, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::SendAvailableObjects, callback));
    
    SubscribeToWebViz();
  }
  
  if (ANKI_VERIFY(_robot->HasGatewayInterface(),
                  "CubeCommsComponent.InitDependent.NoGatewayInterface", "")) {
    auto* gatewayInterface = _robot->GetGatewayInterface();
    auto callback = std::bind(&CubeCommsComponent::HandleAppEvents, this, std::placeholders::_1);
    _signalHandles.push_back(gatewayInterface->Subscribe(external_interface::GatewayWrapperTag::kCubesAvailableRequest, callback));
  }
  
  const auto* platform = robot->GetContextDataPlatform();
  if (ANKI_VERIFY(platform != nullptr,
                  "CubeCommsComponent.InitDependent.DataPlatformIsNull",
                  "Null data platform!")) {
    // Grab the preferred cube from file
    ReadPreferredCubeFromFile();
    
    // Tell CubeBleClient where to find the cube firmware for OTA
    const auto& cubeFirmwarePath = platform->pathToResource(Util::Data::Scope::Resources, kCubeFirmwarePath);
    if (Util::FileUtils::FileExists(cubeFirmwarePath)) {
      _cubeBleClient->SetCubeFirmwareFilepath(cubeFirmwarePath);
    } else {
      PRINT_NAMED_ERROR("CubeCommsComponent.InitDependent.MissingCubeFirmware",
                        "Missing cube dfu firmware file! Should be at %s",
                        cubeFirmwarePath.c_str());
    }
  }

  if (!_cubeBleClient->Init()) {
    PRINT_NAMED_ERROR("CubeCommsComponent.InitDependent.FailedToInitBleClient",
                      "Failed to initialize cubeBleClient");
  }
}


void CubeCommsComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // Update the CubeBleClient instance
  if (!_cubeBleClient->Update()) {
    PRINT_NAMED_ERROR("CubeCommsComponent.UpdateDependent.CubeBleUpdateFailed",
                      "Failed updating CubeBleClient");
    return;
  }
  
  // Send info to web viz occasionally
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (now_sec > _nextSendWebVizDataTime_sec) {
    SendDataToWebViz();
    _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
  }
  
  // See if we should kick off a scan for cubes once the
  // pending disconnection completes.
  if (_startScanWhenUnconnected) {
    if (GetCubeConnectionState() == CubeConnectionState::UnconnectedIdle) {
      PRINT_NAMED_INFO("CubeCommsComponent.UpdateDependent.StartingScan",
                       "Cube has fully disconnected and a scan was scheculed. "
                       "Starting a scan for cubes, which should result in a cube connection");
      _startScanWhenUnconnected = false;
      if (!StartScanningForCubes()) {
        PRINT_NAMED_WARNING("CubeCommsComponent.UpdateDependent.FailedInitiateScan",
                            "Failed to initiate scanning for cubes");
      }
    } else if (GetCubeConnectionState() != CubeConnectionState::PendingDisconnect) {
      DEV_ASSERT_MSG(false,
                     "CubeCommsComponent.UpdateDependent.UnexpectedConnectionState",
                     "A scan is scheduled (_startScanWhenUnconnected == true), but we are not pending a disconnection. Connection state: %s",
                     CubeConnectionStateToString(GetCubeConnectionState()));
      _startScanWhenUnconnected = false;
    }
  }
  
  // See if we have a scheduled disconnection
  if ((_disconnectFromCubeTime_sec > 0.f) &&
      (now_sec > _disconnectFromCubeTime_sec)) {
    if (!_cubeBleClient->RequestDisconnectFromCube()) {
      // Inform callbacks of failure to request a disconnect
      for (auto& callback : _disconnectedCallbacks) {
        callback(false);
      }
      _disconnectedCallbacks.clear();
    }
    _disconnectFromCubeTime_sec = -1.f;
  }
}


bool CubeCommsComponent::RequestConnectToCube(const ConnectionCallback& connectedCallback)
{
  // If a disconnection is scheduled, un-schedule it and return.
  if (_disconnectFromCubeTime_sec > 0.f) {
    _disconnectFromCubeTime_sec = -1.f;
    DEV_ASSERT(IsConnectedToCube(), "CubeCommsComponent.RequestConnectToCube.ShouldBeConnectedIfDisconnectScheduled");
    PRINT_NAMED_INFO("CubeCommsComponent.RequestConnectToCube.CancellingScheduledDisconnect",
                     "We are already connected to this cube, but a disconnection was scheduled. Cancelling the scheduled disconnection.");
    if (connectedCallback != nullptr) {
      connectedCallback(true);
    }
    return true;
  }
  
  // Handle this request appropriately based on the current connection state
  const auto currConnectionState = GetCubeConnectionState();
  switch(currConnectionState) {
    case CubeConnectionState::Connected:
    case CubeConnectionState::PendingConnect:
    {
      PRINT_NAMED_WARNING("CubeCommsComponent.RequestConnectToCube.AlreadyConnected",
                         "Already connected or pending connection to cube %s (active ID %d). Connection state %s",
                         GetCurrentCube().c_str(),
                         GetConnectedCubeActiveId(),
                         CubeConnectionStateToString(GetCubeConnectionState()));
      return false;
    }
    case CubeConnectionState::ScanningForCubes:
    {
      PRINT_NAMED_WARNING("CubeCommsComponent.RequestConnectToCube.AlreadyScanning",
                          "Already scanning for cubes");
      return false;
    }
    case CubeConnectionState::UnconnectedIdle:
    {
      // Kick off scanning process, which will end with a
      // cube connection if successful.
      if (!StartScanningForCubes()) {
        PRINT_NAMED_WARNING("CubeCommsComponent.RequestConnectToCube.FailedInitiateScan",
                            "Failed to initiate scanning for cubes");
        return false;
      }
      break;
    }
    case CubeConnectionState::PendingDisconnect:
    {
      // Plan to start scanning when we fully disconnect from the cube
      _startScanWhenUnconnected = true;
      break;
    }
    default:
    {
      PRINT_NAMED_WARNING("CubeCommsComponent.RequestConnectToCube.InvalidConnectionState",
                         "Cannot request a cube connection - invalid connection state %s",
                         CubeConnectionStateToString(GetCubeConnectionState()));
      return false;
    }
  }
  
  if (connectedCallback != nullptr) {
    _connectedCallbacks.push_back(connectedCallback);
  }
  
  return true;
}


bool CubeCommsComponent::RequestDisconnectFromCube(const float gracePeriod_sec, const ConnectionCallback& disconnectedCallback)
{
  if (GetCubeConnectionState() == CubeConnectionState::UnconnectedIdle) {
    PRINT_NAMED_WARNING("CubeCommsComponent.RequestDisconnectFromCube.Unconnected",
                        "Already unconnected!");
    return false;
  }
  
  // If a scan for cubes was scheduled, cancel it
  _startScanWhenUnconnected = false;
  
  if (gracePeriod_sec <= 0.f) {
    if (!_cubeBleClient->RequestDisconnectFromCube()) {
      return false;
    }
  } else {
    const auto now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _disconnectFromCubeTime_sec = now_sec + gracePeriod_sec;
    PRINT_NAMED_INFO("CubeCommsComponent.RequestDisconnectFromCube.DisconnectScheduled",
                     "Disconnect scheduled to occur in %.2f seconds. Cube will remain connected in the interim.",
                     gracePeriod_sec);
  }
  
  if (disconnectedCallback != nullptr) {
    _disconnectedCallbacks.push_back(disconnectedCallback);
  }
  return true;
}


void CubeCommsComponent::SetPreferredCube(const BleFactoryId& factoryId)
{
  _preferredCubeFactoryId = factoryId;
  
  // Save this preference to disk
  const auto* platform = _robot->GetContextDataPlatform();
  if (ANKI_VERIFY(platform != nullptr,
                  "CubeCommsComponent.SetPreferredCube.NullDataPlatform", "")) {
    // First write to a temporary file, then rename it to kPreferredCubeFilename
    const auto& preferredCubeFilePath = platform->pathToResource(Util::Data::Scope::Persistent, kPreferredCubeFilename);
    const auto& tmpPreferredCubeFilePath = preferredCubeFilePath + ".tmp";
    Util::JsonWriter writer(tmpPreferredCubeFilePath);
    writer.AddEntry(kBlockFacIdKey, _preferredCubeFactoryId);
    writer.Close();
    if (0 != rename(tmpPreferredCubeFilePath.c_str(), preferredCubeFilePath.c_str())) {
      Util::FileUtils::DeleteFile(tmpPreferredCubeFilePath);
    }
  }
}


void CubeCommsComponent::ForgetPreferredCube()
{
  _preferredCubeFactoryId.clear();
  SetPreferredCube(_preferredCubeFactoryId);
}

  
bool CubeCommsComponent::SendCubeLights(const CubeLights& cubeLights)
{
  CubeLightSequence cubeLightSequence;
  std::vector<CubeLightKeyframeChunk> cubeLightKeyframeChunks;
  
  GenerateCubeLightMessages(cubeLights,
                            cubeLightSequence,
                            cubeLightKeyframeChunks);
  
  for (auto& keyframeMsg : cubeLightKeyframeChunks) {
    MessageEngineToCube cubeKeyframeChunkMsg(std::move(keyframeMsg));
    if (!SendCubeMessage(cubeKeyframeChunkMsg)) {
      PRINT_NAMED_WARNING("CubeCommsComponent.SendCubeLights.FailedSendingChunk",
                          "Failed to send CubeLightKeyframeChunk message (starting index %d)",
                          keyframeMsg.startingIndex);
      return false;
    }
  }
  MessageEngineToCube cubeLightSequenceMsg(std::move(cubeLightSequence));
  if (!SendCubeMessage(cubeLightSequenceMsg)) {
    PRINT_NAMED_WARNING("CubeCommsComponent.SendCubeLights.FailedSendingSequence",
                        "Failed to send CubeLightSequence message");
    return false;
  }
  return true;
}


ActiveID CubeCommsComponent::GetConnectedCubeActiveId() const
{
  auto activeId = ObservableObject::InvalidActiveID;
  if (IsConnectedToCube()) {
    activeId = GetActiveId(GetCurrentCube());
  }
  return activeId;
}


bool CubeCommsComponent::StartScanningForCubes(const bool autoConnectAfterScan)
{
  if (GetCubeConnectionState() == CubeConnectionState::ScanningForCubes) {
    PRINT_NAMED_WARNING("CubeCommsComponent.StartScanningForCubes.AlreadyScanning",
                        "Already scanning for cubes - not initiating scan");
    return false;
  }
  
  if (IsConnectedToCube() ||
      GetCubeConnectionState() == CubeConnectionState::PendingConnect) {
    PRINT_NAMED_WARNING("CubeCommsComponent.StartScanningForCubes.AlreadyConnected",
                        "Already connected or pending connection to cube %s - not initiating scan (connection state %s)",
                        GetCurrentCube().c_str(),
                        CubeConnectionStateToString(GetCubeConnectionState()));
    return false;
  }
  
  _connectAfterScan = autoConnectAfterScan;
  
  PRINT_NAMED_INFO("CubeCommsComponent.StartScanningForCubes.StartScan",
                   "Beginning scan for cubes (duration %.2f seconds). Will %sattempt to connect to a cube after scan.",
                   kDefaultDiscoveryTime_sec,
                   _connectAfterScan ? "" : "NOT ");
  
  _cubeScanResults.clear();
  _cubeBleClient->SetScanDuration(kDefaultDiscoveryTime_sec);
  _cubeBleClient->StartScanning();
  
  return true;
}


void CubeCommsComponent::StopScanningForCubes()
{
  if (GetCubeConnectionState() != CubeConnectionState::ScanningForCubes) {
    PRINT_NAMED_WARNING("CubeCommsComponent.StopScanningForCubes.NotScanning",
                        "StopScanning requested, but we are not currently scanning for cubes! Connection state: %s",
                        CubeConnectionStateToString(GetCubeConnectionState()));
  }
  
  PRINT_NAMED_INFO("CubeCommsComponent.StopScanningForCubes.StopScan",
                   "Stopping scan for cubes.");
  
  _cubeBleClient->StopScanning();
}


bool CubeCommsComponent::SendCubeMessage(const MessageEngineToCube& msg)
{
  const bool res = _cubeBleClient->SendMessageToLightCube(msg);
  return res;
}


void CubeCommsComponent::GenerateCubeLightMessages(const CubeLights& cubeLights,
                                                   CubeLightSequence& cubeLightSequence,
                                                   std::vector<CubeLightKeyframeChunk>& cubeLightKeyframeChunks)
{
  DEV_ASSERT(cubeLightKeyframeChunks.empty(), "CubeCommsComponent.GenerateCubeLightMessages.CubeLightKeyframeChunksNotEmpty");
  cubeLightKeyframeChunks.clear();

  DEV_ASSERT(!(cubeLights.playOnce && cubeLights.rotate), "CubeCommsComponent.GenerateCubeLightMessages.CannotHaveBothPlayOnceAndRotation");

  int baseIndex = 0;
  std::vector<LedAnimation> animations;
  for (const auto& lightState : cubeLights.lights) {
    LedAnimation anim(lightState, baseIndex);
    if (cubeLights.playOnce) {
      anim.SetPlayOnce();
    }
    animations.push_back(anim);
    // Increment baseIndex so that the next LedAnimation is created
    // with the proper base index
    baseIndex += anim.GetKeyframes().size();
  }

  // There should be one LedAnimation for each LED on the cube
  DEV_ASSERT(animations.size() == kNumCubeLEDs, "CubeCommsComponent.GenerateCubeLightMessages.WrongNumAnimations");

  // If rotation is specified, then link the animations appropriately,
  // wrapping around if necessary.
  if (cubeLights.rotate) {
    for (int i=0 ; i<kNumCubeLEDs ; i++) {
      animations[(i + 1) % kNumCubeLEDs].LinkToOther(animations[i]);
    }
  }

  // Create CubeLightSequence message, which indicates the starting keyframe
  // indices for each LED.
  cubeLightSequence.flags = 0;
  size_t ledIndex = 0;
  for (const auto& anim : animations) {
    cubeLightSequence.initialIndex[ledIndex++] = anim.GetStartingIndex();
  }

  // Loop over all led animations, and separate chunks of 3 into messages. This
  // essentially concatenates the keyframes of all 4 LED animations then separates
  // the entire list into chunks of 3 for sending over the wire.
  const int chunkSize = 3;
  int keyframeIndex = 0;
  for (const auto& animation : animations) {
    for (const auto& keyframe : animation.GetKeyframes()) {
      const int innerIndex = (keyframeIndex % chunkSize);
      if (innerIndex == 0) {
        // Time to append a new CubeLightKeyframeChunk message
        cubeLightKeyframeChunks.emplace_back();
        cubeLightKeyframeChunks.back().startingIndex = keyframeIndex;
      }
      CubeLightKeyframeChunk& currChunk = cubeLightKeyframeChunks.back();
      currChunk.keyframes[innerIndex] = keyframe;

      ++keyframeIndex;
    }
  }
}


void CubeCommsComponent::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& tag = event.GetData().GetTag();
  switch(tag) {
    case ExternalInterface::MessageGameToEngineTag::ConnectToCube:
      RequestConnectToCube();
      break;
    case ExternalInterface::MessageGameToEngineTag::DisconnectFromCube:
      RequestDisconnectFromCube(event.GetData().Get_DisconnectFromCube().gracePeriod_sec);
      break;
    case ExternalInterface::MessageGameToEngineTag::ForgetPreferredCube:
      ForgetPreferredCube();
      break;
    case ExternalInterface::MessageGameToEngineTag::SetPreferredCube:
      SetPreferredCube(event.GetData().Get_SetPreferredCube().factoryId);
      break;
    case ExternalInterface::MessageGameToEngineTag::SendAvailableObjects:
      _broadcastObjectAvailableMsg = event.GetData().Get_SendAvailableObjects().enable;
      break;
    default:
      PRINT_NAMED_ERROR("CubeCommsComponent.HandleGameEvents.UnhandledTag",
                        "Tag %s is not handled!",
                        MessageGameToEngineTagToString(tag));
      break;
  }
}
  
void CubeCommsComponent::HandleAppEvents(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kCubesAvailableRequest ) {
    if( _robot->HasGatewayInterface() ) {
      auto* msg = new external_interface::CubesAvailableResponse;
      msg->mutable_factory_ids()->Reserve( (int)_cubeScanResults.size() );
      int i=0;
      for( const auto& p : _cubeScanResults ) {
        msg->mutable_factory_ids()->Add();
        (*msg->mutable_factory_ids())[i] = p.first;
        ++i;
      }
      auto* gi = _robot->GetGatewayInterface();
      gi->Broadcast( ExternalMessageRouter::WrapResponse( msg ) );
    }
  }
}


void CubeCommsComponent::HandleObjectAvailable(const ExternalInterface::ObjectAvailable& msg)
{
  // Ensure that this message is referring to the expected light cube type
  DEV_ASSERT_MSG(msg.objectType == kValidCubeType,
                 "CubeCommsComponent.HandleObjectAvailable.InvalidType",
                 "%s is not a valid object type. We expect to hear only from objects of type %s",
                 ObjectTypeToString(msg.objectType),
                 ObjectTypeToString(kValidCubeType));

  const bool cubeUnknown = (_cubeScanResults.find(msg.factory_id) == _cubeScanResults.end());
  _cubeScanResults[msg.factory_id] = msg.rssi;
  
  if( cubeUnknown || _broadcastObjectAvailableMsg ) {
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(ObjectAvailable(msg)));
  }
  
  // If this is our preferred cube, then we can stop scanning right away
  if (msg.factory_id == _preferredCubeFactoryId) {
    PRINT_NAMED_INFO("CubeCommsComponent.HandleObjectAvailable.EndingScanEarly",
                     "Ending cube scan early since we have heard from our preferred cube '%s'",
                     _preferredCubeFactoryId.c_str());
    StopScanningForCubes();
  }
}


void CubeCommsComponent::HandleCubeMessage(const BleFactoryId& factoryId, const MessageCubeToEngine& msg)
{
  const auto activeId = GetActiveId(factoryId);
  if (activeId == ObservableObject::InvalidActiveID) {
    PRINT_NAMED_ERROR("CubeCommsComponent.HandleCubeMessage.NoActiveId",
                      "Could not find ActiveId for block with factory ID %s",
                      factoryId.c_str());
    return;
  }

  switch (msg.GetTag()) {
    case MessageCubeToEngineTag::accelData:
    {
      _robot->GetCubeAccelComponent().HandleCubeAccelData(activeId, msg.Get_accelData());
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("CubeCommsComponent.HandleCubeMessage.UnhandledTag",
                        "Unhandled tag %s (factoryId %s)",
                        MessageCubeToEngineTagToString(msg.GetTag()),
                        factoryId.c_str());
      break;
    }
  }
}


void CubeCommsComponent::HandleConnectionStateChange(const BleFactoryId& factoryId, const bool connected)
{
  PRINT_NAMED_INFO("CubeCommsComponent.HandleConnectionStateChange.Recvd", "FactoryID %s, connected %d",
                   factoryId.c_str(), connected);
  
  if (connected) {
    OnCubeConnected(factoryId);
  } else {
    OnCubeDisconnected(factoryId);
  }
}


void CubeCommsComponent::OnCubeConnected(const BleFactoryId& factoryId)
{
  if (_factoryIdToActiveIdMap.find(factoryId) == _factoryIdToActiveIdMap.end()) {
    // Must be the first time we're connecting to this cube. Generate a new ActiveID.
    _factoryIdToActiveIdMap[factoryId] = GetNextActiveId();
  }
  
  const auto& activeId = _factoryIdToActiveIdMap[factoryId];
  
  // log event to das
  Anki::Util::sInfoF("robot.accessory_connection", {{DDATA,"connected"}}, "%s,%s",
                     factoryId.c_str(), EnumToString(kValidCubeType));
  
  // Add active object to blockworld
  const ObjectID objID = _robot->GetBlockWorld().AddConnectedActiveObject(activeId, factoryId, kValidCubeType);
  if (objID.IsSet()) {
    PRINT_NAMED_INFO("CubeCommsComponent.OnCubeConnected.Connected",
                     "Object %d (activeID %d, factoryID %s)",
                     objID.GetValue(), activeId, factoryId.c_str());
  }
  
  // Viz info
  _robot->GetContext()->GetVizManager()->SendObjectConnectionState(activeId, kValidCubeType, true);
  
  // TODO: arguably blockworld should do this, because when do we want to remove/add objects and not notify?
  if (objID.IsSet()) {
    // Send connection message to game
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(ObjectConnectionState(objID.GetValue(),
                                                                factoryId,
                                                                kValidCubeType,
                                                                true)));
  }
  
  // Save this cube as the preferred cube to connect to if
  // we don't already have one.
  if(_preferredCubeFactoryId.empty()) {
    SetPreferredCube(factoryId);
  }
  
  // Call callbacks
  for (auto& callback : _connectedCallbacks) {
    callback(true);
  }
  _connectedCallbacks.clear();
}


void CubeCommsComponent::OnCubeDisconnected(const BleFactoryId& factoryId)
{
  // log event to das
  Anki::Util::sInfoF("robot.accessory_connection", {{DDATA,"disconnected"}}, "%s,%s",
                     factoryId.c_str(), EnumToString(kValidCubeType));
  
  const auto& activeId = GetActiveId(factoryId);
  
  if (activeId == ObservableObject::InvalidActiveID) {
    // We do not have an active ID for this factory ID. Wtf?
    PRINT_NAMED_ERROR("CubeCommsComponent.OnCubeDisconnected.NoActiveId",
                      "Could not find an active ID for cube with factory ID %s",
                      factoryId.c_str());
    return;
  }
  
  // Remove active object from blockworld if it exists, and remove all instances in all origins
  const ObjectID objID = _robot->GetBlockWorld().RemoveConnectedActiveObject(activeId);
  
  // Update viz info
  _robot->GetContext()->GetVizManager()->SendObjectConnectionState(activeId, kValidCubeType, false);
  
  // TODO: arguably blockworld should do this, because when do we want to remove/add objects and not notify?
  if (objID.IsSet()) {
    // Send connection message to game
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(ObjectConnectionState(objID.GetValue(),
                                                                factoryId,
                                                                kValidCubeType,
                                                                false)));
  }
  
  // Call callbacks
  for (auto& callback : _disconnectedCallbacks) {
    callback(true);
  }
  _disconnectedCallbacks.clear();
}


void CubeCommsComponent::HandleScanForCubesFinished()
{
  if (_cubeScanResults.empty()) {
    PRINT_NAMED_INFO("CubeCommsComponent.HandleScanForCubesFinished.NoCubesFound",
                     "List of available cubes is empty - no advertising cubes were found during scanning. Sad!");
    for (const auto& callback : _connectedCallbacks) {
      callback(false);
    }
    _connectedCallbacks.clear();
    return;
  }
  
  PRINT_NAMED_INFO("CubeCommsComponent.HandleScanForCubesFinished.ScanningForCubesEnded",
                   "Done scanning for cubes. Number of available cubes %zu",
                   _cubeScanResults.size());
  
  if (!_connectAfterScan) {
    PRINT_NAMED_INFO("CubeCommsComponent.HandleScanForCubesFinished.IgnoringScanResults",
                     "Scanning has completed but _connectAfterScan is false, so we will not attempt to connect to a cube");
    return;
  }
  
  // Discovery period has ended. Loop over list of available cubes and connect to
  // the preferred cube if possible, else the cube with highest RSSI.
  BleFactoryId cubeToConnectTo = "";
  auto maxRssiSeen = std::numeric_limits<int>::min();
  for (const auto& mapEntry : _cubeScanResults) {
    const auto& facId = mapEntry.first;
    const auto& rssi = mapEntry.second;
    
    if (facId == _preferredCubeFactoryId) {
      cubeToConnectTo = facId;
      break;
    } else if (rssi > maxRssiSeen) {
      cubeToConnectTo = facId;
      maxRssiSeen = rssi;
    }
  }
  
  DEV_ASSERT(!cubeToConnectTo.empty(), "CubeCommsComponent.HandleScanForCubesFinished.NoCubeChosen");
  
  PRINT_NAMED_INFO("CubeCommsComponent.HandleScanForCubesFinished.AttemptingConnection",
                   "Attempting to connect to cube with factoryID %s because %s. Signal strength %d.",
                   cubeToConnectTo.c_str(),
                   (cubeToConnectTo == _preferredCubeFactoryId) ?
                     "it is the preferred cube" :
                     "it had the highest signal strength",
                   _cubeScanResults[cubeToConnectTo]);
  
  if (!_cubeBleClient->RequestConnectToCube(cubeToConnectTo)) {
    PRINT_NAMED_WARNING("CubeCommsComponent.HandleScanForCubesFinished.FailedRequestingConnection",
                        "Request to connect to cube with factory ID %s failed",
                        cubeToConnectTo.c_str());
  }
}


void CubeCommsComponent::HandleConnectionFailed(const BleFactoryId& factoryId)
{
  PRINT_NAMED_WARNING("CubeCommsComponent.HandleConnectionFailed.ConnectionAttemptFailed",
                      "Connection attempt to cube %s has failed",
                      factoryId.c_str());
  
  // Inform callbacks of failure to connect
  for (auto& callback : _connectedCallbacks) {
    callback(false);
  }
  _connectedCallbacks.clear();
}


ActiveID CubeCommsComponent::GetNextActiveId() const
{
  static ActiveID nextActiveId = 1;
  return nextActiveId++;
}


ActiveID CubeCommsComponent::GetActiveId(const BleFactoryId& factoryId) const
{
  auto activeID = ObservableObject::InvalidActiveID;
  const auto findIt = _factoryIdToActiveIdMap.find(factoryId);
  if (findIt != _factoryIdToActiveIdMap.end()) {
    activeID = findIt->second;
  }
  return activeID;
}


void CubeCommsComponent::ReadPreferredCubeFromFile()
{
  _preferredCubeFactoryId.clear();

  // Read preferred cube from file. Create empty preferred cube
  // file if missing or json wasn't parsed correctly
  const auto* platform = _robot->GetContextDataPlatform();
  if (ANKI_VERIFY(platform != nullptr,
                  "CubeCommsComponent.ReadPreferredCubeFromFile.NullDataPlatform", "")) {
    const auto& preferredCubeFile = platform->pathToResource(Util::Data::Scope::Persistent, kPreferredCubeFilename);
    bool parsingSuccessful = false;
    if(Util::FileUtils::FileExists(preferredCubeFile)) {
      const auto& contents = Util::FileUtils::ReadFile(preferredCubeFile);
      Json::Value root;
      Json::Reader reader;
      if(reader.parse(contents, root)) {
        parsingSuccessful = JsonTools::GetValueOptional(root, kBlockFacIdKey, _preferredCubeFactoryId);
      }
    }

    if(!parsingSuccessful) {
      PRINT_NAMED_WARNING("CubeCommsComponent.ReadPreferredCubeFromFile.FailedLoadingPreferredCubeFromFile",
                          "Creating a new file with no preferred cube");
      SetPreferredCube("");
    }
  }
}

void CubeCommsComponent::SubscribeToWebViz()
{
  if (!ANKI_DEV_CHEATS) {
    return;
  }
  
  const auto* context = _robot->GetContext();
  if (context != nullptr) {
    auto* webService = context->GetWebService();
    if (webService != nullptr) {
      auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
        const auto& flashCubeLights = data["flashCubeLights"];
        const bool shouldFlashCubeLights = flashCubeLights.isBool() && flashCubeLights.asBool();
        if(shouldFlashCubeLights && IsConnectedToCube()) {
          // flash lights on connected cube
          const auto& activeId = GetActiveId(GetCurrentCube());
          const auto* object = _robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(activeId);
          if (object != nullptr) {
            _robot->GetCubeLightComponent().PlayLightAnimByTrigger(object->GetID(), CubeAnimationTrigger::Flash);
          }
        }
        
        const auto& connectCube = data["connectCube"];
        const bool shouldConnectCube = connectCube.isBool() && connectCube.asBool();
        if(shouldConnectCube) {
          RequestConnectToCube();
        }
        
        const auto& disconnectCube = data["disconnectCube"];
        const bool shouldDisconnectCube = disconnectCube.isBool() && disconnectCube.asBool();
        if(shouldDisconnectCube) {
          RequestDisconnectFromCube(0.f);
        }
        
        const auto& forgetPreferredCube = data["forgetPreferredCube"];
        const bool shouldForgetCube = forgetPreferredCube.isBool() && forgetPreferredCube.asBool();
        if(shouldForgetCube) {
          ForgetPreferredCube();
        }
      };
      
      _signalHandles.emplace_back(webService->OnWebVizData(kWebVizModuleNameCubes).ScopedSubscribe(onData));
    }
  }
}


void CubeCommsComponent::SendDataToWebViz()
{
  if (!ANKI_DEV_CHEATS) {
    return;
  }
  
  const auto* context = _robot->GetContext();
  if (context != nullptr) {
    auto* webService = context->GetWebService();
    if (webService!= nullptr) {
      Json::Value toSend = Json::objectValue;
      Json::Value commInfo = Json::objectValue;
      commInfo["connectionState"] = CubeConnectionStateToString(GetCubeConnectionState());
      commInfo["connectedCube"] = IsConnectedToCube() ? GetCurrentCube() : "(none)";
      commInfo["preferredCube"] = _preferredCubeFactoryId.empty() ? "(none)" : _preferredCubeFactoryId;
      
      Json::Value cubeData = Json::arrayValue;
      for (const auto& mapEntry : _cubeScanResults) {
        const auto& factoryId = mapEntry.first;
        const auto& rssi = mapEntry.second;
        Json::Value blob;
        blob["address"]   = factoryId;
        blob["lastRssi"]  = rssi;
        cubeData.append(blob);
      }
      commInfo["cubeData"] = cubeData;
      toSend["commInfo"] = commInfo;
      webService->SendToWebViz(kWebVizModuleNameCubes, toSend);
    }
  }
}


} // Cozmo namespace
} // Anki namespace
