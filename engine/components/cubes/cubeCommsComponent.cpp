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
#include "engine/components/cubeAccelComponent.h"
#include "engine/components/cubes/ledAnimation.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotToEngineImplMessaging.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/cubeMessages.h"
#include "clad/externalInterface/lightCubeMessage.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  // How long to remain in discovery mode
  const float kDefaultDiscoveryTime_sec = 1.f;
  
  // How often do we check for disconnected objects
  const float kDisconnectCheckPeriod_sec = 2.0f;
  
  // How long must the object be disconnected before we really remove it from the list of connected objects
  const float kDisconnectTimeout_sec = 2.0f;
  
  const int kNumCubeLeds = Util::EnumToUnderlying(CubeConstants::NUM_CUBE_LEDS);
}

  
CubeCommsComponent::CubeCommsComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeComms)
, _cubeBleClient(CubeBleClient::GetInstance())
{
  // Register callbacks for messages from CubeBleClient
  _cubeBleClient->RegisterObjectAvailableCallback(std::bind(&CubeCommsComponent::HandleObjectAvailable, this, std::placeholders::_1));
  _cubeBleClient->RegisterLightCubeMessageCallback(std::bind(&CubeCommsComponent::HandleLightCubeMessage, this, std::placeholders::_1, std::placeholders::_2));
  _cubeBleClient->RegisterCubeConnectedCallback([this](const BleFactoryId& factoryId) { this->HandleConnectionStateChange(factoryId, true); });
  _cubeBleClient->RegisterCubeDisconnectedCallback([this](const BleFactoryId& factoryId) { this->HandleConnectionStateChange(factoryId, false); });
}


void CubeCommsComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  // Game to engine message handling:
  if (_robot->HasExternalInterface()) {
    auto callback = std::bind(&CubeCommsComponent::HandleGameEvents, this, std::placeholders::_1);
    auto* extInterface = _robot->GetExternalInterface();
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolResetMessage, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::SendAvailableObjects, callback));
  }
}


void CubeCommsComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // Update the CubeBleClient instance
  const auto& result = _cubeBleClient->Update();
  if (result != RESULT_OK) {
    PRINT_NAMED_ERROR("CubeCommsComponent.Update.CubeBleUpdateFailed",
                      "Failed updating CubeBleClient (result %d)",
                      Util::EnumToUnderlying(result));
    return;
  }
  
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // If we're connected to a cube, then it's assumed that we're communicating with
  // it. So just update the lastHeardTime_sec here to prevent accidental removal
  // in the disconnection check logic below when Discovery mode is first started.
  for (auto& mapEntry : _availableCubes) {
    auto& cube = mapEntry.second;
    if (cube.connected) {
      cube.lastHeardTime_sec = now_sec;
    }
  }
  
  // Every once in a while, ensure we've heard from all the cubes recently
  // and remove stale cubes from the list if not.
  if (now_sec > _nextDisconnectCheckTime_sec) {
    for (auto it = _availableCubes.cbegin() ; it != _availableCubes.cend() ; ) {
      // Remove the cube from the list if we're not connected to it and haven't
      // heard from it in a while:
      const auto& cube = it->second;
      if (!cube.connected && (now_sec - cube.lastHeardTime_sec > kDisconnectTimeout_sec)) {
        PRINT_NAMED_INFO("CubeCommsComponent.Update.RemovingStaleCube",
                         "Removing unconnected cube with factory ID %d since we haven't heard from it recently.",
                         cube.factoryId);
        _factoryIdToActiveIdMap.erase(cube.factoryId);
        it = _availableCubes.erase(it);
        
        if (_broadcastObjectAvailableMsg) {
          _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ObjectUnavailable(cube.factoryId)));
        }
      } else {
        ++it;
      }
    }
    
    _nextDisconnectCheckTime_sec = now_sec + kDisconnectCheckPeriod_sec;
  }

  // Update discovering
  if (_discovering) {
    // Check if discovery period is over:
    if (now_sec > _discoveringEndTime_sec) {
      // Discovery period has ended. Loop over list of available cubes and connect to
      // as many as possible, preferring those with high RSSI.
      
      // For each object type, keep track of the max rssi seen for that type
      std::map<ObjectType, int> maxRssiByType;
      
      // Keep track of which cubes to connect to. Map is keyed on ObjectType to
      // ensure we only connect to _one_ object of a given type.
      std::map<ObjectType, BleFactoryId> objectsToConnectTo;
      
      for (const auto& mapEntry : _availableCubes) {
        const auto& cube = mapEntry.second;
        const auto& type = cube.objectType;
        const auto& rssi = cube.lastRssi;

        // If this is the first cube of this type to be seen or if this cube's rssi
        // is higher than the max seen, add it to the list of cubes to connect to
        if ((maxRssiByType.find(type) == maxRssiByType.end()) ||
            (rssi > maxRssiByType[type])) {
          maxRssiByType[type] = rssi;
          objectsToConnectTo[type] = cube.factoryId;;
        }
      }
      
      // Connect to the selected objects
      for (const auto& entry : objectsToConnectTo) {
        const auto& factoryId = entry.second;
        if (!_cubeBleClient->ConnectToCube(factoryId)) {
          PRINT_NAMED_WARNING("CubeCommsComponent.Update.FailedConnecting", "Failed connecting to cube with factory ID %d", factoryId);
        }
      }
      
      // Done discovering
      EnableDiscovery(false);
    }
  }
}


void CubeCommsComponent::EnableDiscovery(const bool enable, const float discoveryTime_sec)
{
  if (enable != _discovering) {
    _discovering = enable;
    if (enable) {
      const float discoveringTime_sec = FLT_NEAR(discoveryTime_sec, 0.f) ? kDefaultDiscoveryTime_sec : discoveryTime_sec;
      _discoveringEndTime_sec = discoveringTime_sec + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      PRINT_NAMED_INFO("CubeCommsComponent.EnableDiscovery.EnableDiscovery",
                       "Enabling discovery mode (duration %.2f seconds)",
                       discoveringTime_sec);
      // Start discovering. Disconnect from all cubes
      for (const auto& mapEntry : _availableCubes) {
        const auto& cube = mapEntry.second;
        if (cube.connected) {
          if (!_cubeBleClient->DisconnectFromCube(cube.factoryId)) {
            PRINT_NAMED_WARNING("CubeCommsComponent.EnableDiscovery.FailedDisconnecting", "Failed disconnecting from cube with factory ID %d", cube.factoryId);
          }
        }
      }
    }
  }
}


bool CubeCommsComponent::SendLightCubeMessage(const ActiveID& activeId, const BlockMessages::LightCubeMessage& msg)
{
  const auto* cube = GetCubeByActiveId(activeId);
  
  if (nullptr == cube) {
    PRINT_NAMED_WARNING("CubeCommsComponent.SendLightCubeMessage.InvalidCube", "Could not find cube with activeID %d", activeId);
    return false;
  }
  if (!cube->connected) {
    PRINT_NAMED_WARNING("CubeCommsComponent.SendLightCubeMessage.NotConnected", "Cannot send message to unconnected to cube (activeID %d)", activeId);
    return false;
  }
  
  const auto factoryId = cube->factoryId;
  const bool res = _cubeBleClient->SendMessageToLightCube(factoryId, msg);
  return res;
}
  
  
// Start/stop ObjectAccel message streaming from the specified cube
// Note: Once we overhaul the cube messaging to only send raw accelerometer
// data and pretty much nothing else, we can get rid of the StreamObjectAccel
// CLAD message, since it would be redundant
bool CubeCommsComponent::SetStreamObjectAccel(const ActiveID& activeId, const bool enable)
{
  const auto cube = GetCubeByActiveId(activeId);
  if ((nullptr != cube) && cube->connected) {
    BlockMessages::LightCubeMessage msg;
    msg.Set_streamObjectAccel(StreamObjectAccel(activeId, enable));
    return _cubeBleClient->SendMessageToLightCube(cube->factoryId, msg);
  }
  return false;
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
  DEV_ASSERT(animations.size() == kNumCubeLeds, "CubeCommsComponent.GenerateCubeLightMessages.WrongNumAnimations");
  
  // If rotation is specified, then link the animations appropriately,
  // wrapping around if necessary.
  if (cubeLights.rotate) {
    for (int i=0 ; i<kNumCubeLeds ; i++) {
      animations[i].LinkToOther(animations[(i + 1) % kNumCubeLeds]);
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


bool CubeCommsComponent::SendCubeLights(const ActiveID& activeId, const CubeLights& cubeLights)
{
  const auto cube = GetCubeByActiveId(activeId);
  bool success = false;
  if ((nullptr != cube) && cube->connected) {
    BlockMessages::LightCubeMessage lightStateMsg;
    lightStateMsg.Set_cubeLights(CubeLights(cubeLights));
    success = _cubeBleClient->SendMessageToLightCube(cube->factoryId, lightStateMsg);
  }
  return success;
}

  
void CubeCommsComponent::SendBlockPoolData() const
{
  // Persistent pool not yet implemented! (VIC-782)
  PRINT_NAMED_WARNING("CubeCommsComponent.SendBlockPoolData.NotImplemented",
                      "Not sending BlockPoolDataMessage - persistent pool  is not yet implemented!");
}
 
  
void CubeCommsComponent::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& tag = event.GetData().GetTag();
  switch(tag) {
    case ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage:
      SendBlockPoolData();
      break;
    case ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage:
      EnableDiscovery(event.GetData().Get_BlockPoolEnabledMessage().enabled,
                      event.GetData().Get_BlockPoolEnabledMessage().discoveryTimeSecs);
      break;
    case ExternalInterface::MessageGameToEngineTag::BlockPoolResetMessage:
      EnableDiscovery(event.GetData().Get_BlockPoolResetMessage().enable, 0);
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

  
void CubeCommsComponent::HandleObjectAvailable(const ObjectAvailable& msg)
{
  // Ensure that this message is referring to a light cube, not some other object
  DEV_ASSERT(IsValidLightCube(msg.objectType, false), "CubeCommsComponent.HandleObjectAvailable.UnknownType");
  
  // Is this cube already in our list?
  const auto it = _factoryIdToActiveIdMap.find(msg.factory_id);
  const bool alreadyInList = (it != _factoryIdToActiveIdMap.end());
  
  if (alreadyInList) {
    // Find this cube in _availableCubes:
    const auto activeId = it->second;
    const auto cube = GetCubeByActiveId(activeId);
    
    DEV_ASSERT(cube != nullptr, "CubeCommsComponent.HandleObjectAvailable.CouldNotFindCube");
    
    // Update lastHeardTime and RSSI:
    cube->lastRssi = msg.rssi;
    cube->lastHeardTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  } else {
    // Add this cube to the list of available cubes
    CubeInfo newCube;
    newCube.factoryId = msg.factory_id;
    newCube.objectType = msg.objectType;
    newCube.lastRssi = msg.rssi;
    newCube.lastHeardTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    newCube.connected = false;
    AddCubeToList(newCube);
  }
  
  if (_broadcastObjectAvailableMsg) {
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectAvailable(msg)));
  }
}

  
void CubeCommsComponent::HandleLightCubeMessage(const BleFactoryId& factoryId, const BlockMessages::LightCubeMessage& lcm)
{
  const auto it = _factoryIdToActiveIdMap.find(factoryId);
  if (it == _factoryIdToActiveIdMap.end()) {
    PRINT_NAMED_WARNING("CubeCommsComponent.HandleLightCubeMessage.NoActiveId", "Could not find ActiveId for block with factory ID %d", factoryId);
    return;
  }
  
  const auto activeId = it->second;
  
  auto* cube = GetCubeByActiveId(activeId);
  DEV_ASSERT(cube != nullptr, "CubeCommsComponent.HandleLightCubeMessage.CubeNotFound");
  
  cube->lastHeardTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch (lcm.GetTag()) {
    case BlockMessages::LightCubeMessageTag::accel:
    {
      // Modify the message to include the correct activeId and timestamp
      ObjectAccel msg(lcm.Get_accel());
      msg.objectID = activeId;
      msg.timestamp = _robot->GetLastMsgTimestamp(); // TODO: Is this accurate enough?
      
      // Pass this to the CubeAccelComponent to handle:
      _robot->GetCubeAccelComponent().HandleObjectAccel(msg);
      break;
    }
    case BlockMessages::LightCubeMessageTag::moved:
    {
      // Call into Blockworld to handle the message.
      ObjectMoved msg(lcm.Get_moved());
      msg.objectID = activeId;
      msg.timestamp = _robot->GetLastMsgTimestamp();
      // This is sort of a hack - should be probably be moved to BlockWorld
      _robot->GetRobotToEngineImplMessaging().HandleActiveObjectMoved(msg, _robot);
      
      _robot->GetBlockTapFilter().HandleActiveObjectMoved(msg);
      break;
    }
    case BlockMessages::LightCubeMessageTag::powerLevel:
    {
      // Call into Blockworld to handle the message.
      ObjectPowerLevel msg(lcm.Get_powerLevel());
      msg.objectID = activeId;
      // This is sort of a hack - should be probably be moved elsewhere
      _robot->GetRobotToEngineImplMessaging().HandleObjectPowerLevel(msg, _robot);
      break;
    }
    case BlockMessages::LightCubeMessageTag::stopped:
    {
      // Call into Blockworld to handle the message.
      ObjectStoppedMoving msg(lcm.Get_stopped());
      msg.objectID = activeId;
      msg.timestamp = _robot->GetLastMsgTimestamp();
      // This is sort of a hack - should be probably be moved to BlockWorld
      _robot->GetRobotToEngineImplMessaging().HandleActiveObjectStopped(msg, _robot);
      
      _robot->GetBlockTapFilter().HandleActiveObjectStopped(msg);
      break;
    }
    case BlockMessages::LightCubeMessageTag::upAxisChanged:
    {
      ObjectUpAxisChanged msg(lcm.Get_upAxisChanged());
      msg.objectID = activeId;
      msg.timestamp = _robot->GetLastMsgTimestamp();
      
      // This is sort of a hack - should be probably be moved to BlockWorld
      _robot->GetRobotToEngineImplMessaging().HandleActiveObjectUpAxisChanged(msg, _robot);
      break;
    }
    case BlockMessages::LightCubeMessageTag::tapped:
    {
      ObjectTapped msg(lcm.Get_tapped());
      msg.objectID = activeId;
      msg.timestamp = _robot->GetLastMsgTimestamp();
      
      _robot->GetBlockTapFilter().HandleActiveObjectTapped(msg);
      break;
    }
    default:
    {
      PRINT_NAMED_WARNING("CubeCommsComponent.HandleLightCubeMessage.UnhandledTag",
                          "Unhandled tag %s (factoryId %d)",
                          BlockMessages::LightCubeMessageTagToString(lcm.GetTag()),
                          factoryId);
      break;
    }
  }
  
}


void CubeCommsComponent::HandleConnectionStateChange(const BleFactoryId& factoryId, const bool connected)
{
  auto cube = GetCubeByFactoryId(factoryId);
  if (cube == nullptr) {
    PRINT_NAMED_WARNING("CubeCommsComponent.HandleCubeConnected.NullCube", "Could not find cube with factory ID %d", factoryId);
    return;
  }
  
  // grab the active ID:
  const auto it = _factoryIdToActiveIdMap.find(factoryId);
  DEV_ASSERT(it != _factoryIdToActiveIdMap.end(), "CubeCommsComponent.HandleCubeConnected.NotFound");
  const auto& activeId = it->second;
  
  // If we're already connected to this cube, don't need to do anything else
  if (connected == cube->connected) {
    PRINT_NAMED_WARNING("CubeCommsComponent.HandleCubeConnected.NoStateChange",
                        "Already %s cube with factory ID %d",
                        connected ? "connected to" : "disconnected from",
                        cube->factoryId);
    return;
  }
  
  cube->connected = connected;
  
  ObjectID objID;
  if (connected)
  {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"connected"}}, "0x%x,%s",
                        cube->factoryId, EnumToString(cube->objectType));
    
    // Add active object to blockworld
    objID = _robot->GetBlockWorld().AddConnectedActiveObject(activeId, cube->factoryId, cube->objectType);
    if (objID.IsSet()) {
      PRINT_NAMED_INFO("CubeCommsComponent.HandleConnectionStateChange.Connected",
                       "Object %d (activeID %d, factoryID 0x%x, objectType '%s')",
                       objID.GetValue(), activeId, cube->factoryId, EnumToString(cube->objectType));
    }
  } else {
    // log event to das
    Anki::Util::sEventF("robot.accessory_connection", {{DDATA,"disconnected"}}, "0x%x,%s",
                        cube->factoryId, EnumToString(cube->objectType));
    
    // Remove active object from blockworld if it exists, and remove all instances in all origins
    objID = _robot->GetBlockWorld().RemoveConnectedActiveObject(activeId);
    
    // TODO: Remove this object from our list or just leave as not connected?
  }
  
  PRINT_NAMED_INFO("CubeCommsComponent.HandleConnectionStateChange.Recvd", "FactoryID 0x%x, connected %d",
                   cube->factoryId, cube->connected);
  
  // Viz info
  _robot->GetContext()->GetVizManager()->SendObjectConnectionState(activeId, cube->objectType, cube->connected);
  
  // TODO: arguably blockworld should do this, because when do we want to remove/add objects and not notify?
  if (objID.IsSet()) {
    // Send connection message to game
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectConnectionState(objID.GetValue(),
                                                                                  cube->factoryId,
                                                                                  cube->objectType,
                                                                                  cube->connected)));
  }
}
  
 
bool CubeCommsComponent::AddCubeToList(const CubeInfo& cube)
{
  // Is this cube already in the list? Check factory ID against existing elements.
  const auto it = std::find_if(_availableCubes.begin(),
                               _availableCubes.end(),
                               [&cube](const std::pair<ActiveID, CubeInfo>& mapItem){ return mapItem.second.factoryId == cube.factoryId; });
  const bool alreadyInList = (it != _availableCubes.end());
  
  if (!alreadyInList) {
    // ActiveID to assign to the next object added to the list (just to make
    // sure each object gets a unique ActiveID)
    static ActiveID nextActiveId = 1;
    
    // Insert this cube into the container
    _availableCubes[nextActiveId] = cube;
    
    // Add to the convenience map:
    _factoryIdToActiveIdMap[cube.factoryId] = nextActiveId;
    
    // increment nextActiveId for next insertion
    ++nextActiveId;
  }
  
  // in dev, ensure that maps are the same size:
  DEV_ASSERT(_availableCubes.size() == _factoryIdToActiveIdMap.size(), "CubeCommsComponent.AddCubeToList.MapSizeMismatch");
  
  return !alreadyInList;
}
  
bool CubeCommsComponent::RemoveCubeFromList(const BleFactoryId& factoryId)
{
  bool success = true;
  
  // Look up the corresponding ActiveID for this cube:
  const auto it = _factoryIdToActiveIdMap.find(factoryId);
  if (it != _factoryIdToActiveIdMap.end()) {
    const auto activeID = it->second;
    const bool removedElement = (_availableCubes.erase(activeID) != 0);
    if (!removedElement) {
      PRINT_NAMED_WARNING("CubeCommsComponent.RemoveCubeFromList.UnknownCube", "Cube with activeID of %d not found!", activeID);
      success = false;
    }
    
    // Remove the factoryId -> ActiveId map element:
    _factoryIdToActiveIdMap.erase(it);
  } else {
    PRINT_NAMED_WARNING("CubeCommsComponent.RemoveCubeFromList.UnknownCube", "Cube with factory ID of %d not found!", factoryId);
    success = false;
  }
  
  // in dev, ensure that maps are the same size:
  DEV_ASSERT(_availableCubes.size() == _factoryIdToActiveIdMap.size(), "CubeCommsComponent.RemoveCubeFromList.MapSizeMismatch");

  return success;
}
 

void CubeCommsComponent::ClearList()
{
  _availableCubes.clear();
  _factoryIdToActiveIdMap.clear();
}


CubeCommsComponent::CubeInfo* CubeCommsComponent::GetCubeByActiveId(const ActiveID& activeId)
{
  const auto it = _availableCubes.find(activeId);
  if (it != _availableCubes.end()) {
    return &(it->second);
  }
  return nullptr;
}


CubeCommsComponent::CubeInfo* CubeCommsComponent::GetCubeByFactoryId(const BleFactoryId& factoryId)
{
  const auto it = _factoryIdToActiveIdMap.find(factoryId);
  if (it != _factoryIdToActiveIdMap.end()) {
    return GetCubeByActiveId(it->second);
  }
  return nullptr;
}
  
  
  
} // Cozmo namespace
} // Anki namespace
