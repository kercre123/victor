/**
 * File: cubeBleClient_mac.cpp
 *
 * Author: Matt Michini
 * Created: 12/1/2017
 *
 * Description:
 *               Defines interface to simulated cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cubeBleClient.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/externalInterface/messageCubeToEngine.h"
#include "clad/externalInterface/messageEngineToCube.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#include <webots/Emitter.hpp>
#include <webots/Receiver.hpp>
#include <webots/Supervisor.hpp>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using cubeBleClient_mac.cpp
#endif

namespace Anki {
namespace Cozmo {

namespace { // "Private members"

  // Has SetSupervisor() been called yet?
  bool _engineSupervisorSet = false;

  // Current supervisor (if any)
  webots::Supervisor* _engineSupervisor = nullptr;
  
  // Receiver for discovery/advertisement
  webots::Receiver* _discoveryReceiver = nullptr;
  
  // Emitter used to talk to all cubes (by switching the channel as needed)
  webots::Emitter* _cubeEmitter = nullptr;
  
  // Webots comm channel used for the discovery emitter/receiver
  constexpr int kDiscoveryChannel = 99;
  
  // Receivers for each cube (index maps to simulated "factory ID" of cube)
  std::vector<webots::Receiver*> _cubeReceivers;
  
  // simulated 'connection' to cube. Just a boolean that gets set to true
  // when clients request a connection to a specific cube.
  std::map<BleFactoryId, bool> _connectedToCube;
  
} // "private" namespace


CubeBleClient::CubeBleClient()
{
  // Ensure that we have a webots supervisor
  DEV_ASSERT(_engineSupervisorSet, "CubeBleClient.NoWebotsSupervisor");
  
  if (nullptr != _engineSupervisor) {
    _discoveryReceiver = _engineSupervisor->getReceiver("discoveryReceiver");
    DEV_ASSERT(_discoveryReceiver != nullptr, "CubeBleClient.NullDiscoveryReceiver");
    _discoveryReceiver->setChannel(kDiscoveryChannel);
    _discoveryReceiver->enable(SIM_CUBE_TIME_STEP_MS);
    
    _cubeEmitter = _engineSupervisor->getEmitter("cubeCommsEmitter");
    DEV_ASSERT(_cubeEmitter != nullptr, "CubeBleClient.NullCubeEmitter");

    const auto* selfNode = _engineSupervisor->getSelf();
    DEV_ASSERT(selfNode != nullptr, "CubeBleClient.NullRootNode");
    const auto* numReceiversField = selfNode->getField("numCubeReceivers");
    DEV_ASSERT(numReceiversField != nullptr, "CubeBleClient.NullRootNode");
    const int numCubeReceivers = numReceiversField->getSFInt32();
    
    for (int i=0 ; i < numCubeReceivers ; i++) {
      auto* rec = _engineSupervisor->getReceiver("cubeCommsReceiver" + std::to_string(i));
      DEV_ASSERT(rec != nullptr, "CubeBleClient.NullCubeReceiver");
      
      // Set channel to i+1, so that they start at 1 (since simulated light cube factory IDs start at 1)
      rec->setChannel(i+1);
      rec->enable(SIM_CUBE_TIME_STEP_MS);
      
      // Add to the list of receivers:
      _cubeReceivers.push_back(rec);
    }
  }
}
  
  
// Returns the single instance of the object.
CubeBleClient* CubeBleClient::GetInstance()
{
  // Did you remember to call SetSupervisor()?
  DEV_ASSERT(_engineSupervisorSet, "cubeBleClient_android.getInstance.NoSupervisorSet");

  // return the single instance from the base class
  return getInstance();
}


void CubeBleClient::SetSupervisor(webots::Supervisor *sup)
{
  _engineSupervisor = sup;
  _engineSupervisorSet = true;
}
  
  
bool CubeBleClient::SendMessageToLightCube(const BleFactoryId& factoryID, const MessageEngineToCube& msg)
{
  DEV_ASSERT(IsConnectedToCube(factoryID), "CubeBleClient.SendMessageToLightCube.CubeNotConnected");
  
  // channel used is 1 + factoryID
  const int channel = factoryID + 1;
  _cubeEmitter->setChannel(channel);
  
  u8 buff[msg.Size()];
  msg.Pack(buff, msg.Size());
  int res = _cubeEmitter->send(buff, (int) msg.Size());
  
  // return value of 1 indicates that the message was successfully queued (see Webots documentation)
  return (res == 1);
}
  
  
bool CubeBleClient::ConnectToCube(const BleFactoryId& factoryId)
{
  DEV_ASSERT(!IsConnectedToCube(factoryId), "CubeBleClient.ConnectToCube.AlreadyConnected");
  
  // Mark as connected and immediately call connected callback
  _connectedToCube[factoryId] = true;
  for (const auto& callback : _cubeConnectedCallbacks) {
    callback(factoryId);
  }
  return true;
}


bool CubeBleClient::DisconnectFromCube(const BleFactoryId& factoryId)
{
  DEV_ASSERT(IsConnectedToCube(factoryId), "CubeBleClient.ConnectToCube.NotConnected");
  
  // Mark as disconnected and immediately call disconnected callback
  _connectedToCube[factoryId] = false;
  for (const auto& callback : _cubeDisconnectedCallbacks) {
    callback(factoryId);
  }
  return true;
}


bool CubeBleClient::IsConnectedToCube(const BleFactoryId& factoryId)
{
  // First check if factoryID exists in _connectedToCube map first.
  // If it's not even in the map, consider this cube not connected.
  if (_connectedToCube.find(factoryId) == _connectedToCube.end()) {
    return false;
  }

  return _connectedToCube[factoryId];
}


Result CubeBleClient::Update()
{
  // Look for discovery/advertising messages:
  while (_discoveryReceiver->getQueueLength() > 0) {
    // Shove the data into a MessageCubeToEngine and call callbacks.
    MessageCubeToEngine cubeMessage((uint8_t *) _discoveryReceiver->getData(), (size_t) _discoveryReceiver->getDataSize());
    const auto sigStrength = _discoveryReceiver->getSignalStrength();
    _discoveryReceiver->nextPacket();
    if (cubeMessage.GetTag() == MessageCubeToEngineTag::available) {
      // Received an advertisement message
      ExternalInterface::ObjectAvailable msg(cubeMessage.Get_available());
      
      // TODO: Convert the webots signal strength to similar values to what we get from the robot.
      // These numbers were just approximated by experimenting on webots
      msg.rssi = sigStrength;
      
      // Call the appropriate callbacks with the modified message, but only if the state is 'disconnected'
      if (!IsConnectedToCube(msg.factory_id)) {
        for (const auto& callback : _objectAvailableCallbacks) {
          callback(msg);
        }
      }
    } else {
      // Unexpected message type on the discovery channel
      PRINT_NAMED_WARNING("CubeBleClient.Update.UnexpectedMsg", "Expected ObjectAvailable but received %s", MessageCubeToEngineTagToString(cubeMessage.GetTag()));
    }
  }

  // Look for messages from the individual light cubes:
  for (const auto& receiever : _cubeReceivers) {
    const int factoryID = receiever->getChannel(); // factory ID is same as channel
    while (receiever->getQueueLength() > 0) {
      MessageCubeToEngine cubeMessage((uint8_t *) receiever->getData(), (size_t) receiever->getDataSize());
      receiever->nextPacket();
      // Received a light cube message. Call the registered callbacks, but only if the state is 'connected'
      if (IsConnectedToCube(factoryID)) {
        for (const auto& callback : _cubeMessageCallbacks) {
          callback(factoryID, cubeMessage);
        }
      }
    }
  }
  
  return RESULT_OK;
}


} // namespace Cozmo
} // namespace Anki
