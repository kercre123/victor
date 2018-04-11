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
  constexpr int kDiscoveryChannel = 0;
  
  // All of the Webots receivers found in the engine proto
  std::vector<webots::Receiver*> _receivers;
  
  // Maps factory ID to webots receiver
  std::map<BleFactoryId, webots::Receiver*> _cubeReceiverMap;
  
  // simulated 'connection' to cube. Just a boolean that gets set to true
  // when clients request a connection to a specific cube.
  std::map<BleFactoryId, bool> _connectedToCube;
  
  // If we're actively scanning for advertising cubes
  bool _scanning = false;
  
  // Whether to start scanning immediately upon connection to
  // the "bluetooth daemon" (which doesn't exist for mac, so
  // this just means that we should start scanning upon Init())
  bool _scanUponConnection = false;
  
  float _scanDuration_sec = 3.f;
  float _scanUntil_sec = 0.f;
  
} // "private" namespace


int GetReceiverChannel(const BleFactoryId& factoryId)
{
  const int channel = (int) (std::hash<std::string>{}(factoryId) & 0x3FFFFFFF);
  return channel;
}
  
int GetEmitterChannel(const BleFactoryId& factoryId)
{
  return 1 + GetReceiverChannel(factoryId);
}


CubeBleClient::CubeBleClient()
{
  // Ensure that we have a webots supervisor
  DEV_ASSERT(_engineSupervisorSet, "CubeBleClient.NoWebotsSupervisor");
  
  if (nullptr != _engineSupervisor) {
    _discoveryReceiver = _engineSupervisor->getReceiver("discoveryReceiver");
    DEV_ASSERT(_discoveryReceiver != nullptr, "CubeBleClient.NullDiscoveryReceiver");
    _discoveryReceiver->setChannel(kDiscoveryChannel);
    _discoveryReceiver->enable(CUBE_TIME_STEP_MS);
    
    _cubeEmitter = _engineSupervisor->getEmitter("cubeCommsEmitter");
    DEV_ASSERT(_cubeEmitter != nullptr, "CubeBleClient.NullCubeEmitter");
    
    // Grab all the available Webots receivers
    const auto* selfNode = _engineSupervisor->getSelf();
    DEV_ASSERT(selfNode != nullptr, "CubeBleClient.NullRootNode");
    const auto* numReceiversField = selfNode->getField("numCubeReceivers");
    DEV_ASSERT(numReceiversField != nullptr, "CubeBleClient.NullNumReceiversField");
    const int numCubeReceivers = numReceiversField->getSFInt32();

    for (int i=0 ; i<numCubeReceivers ; i++) {
      auto* receiver = _engineSupervisor->getReceiver("cubeCommsReceiver" + std::to_string(i));
      DEV_ASSERT(receiver != nullptr, "CubeBleClient.NullReceiver");
      _receivers.push_back(receiver);
    }
    DEV_ASSERT(!_receivers.empty(), "CubeBleClient.NoReceiversFound");
  }
}


CubeBleClient::~CubeBleClient()
{

}


void CubeBleClient::SetSupervisor(webots::Supervisor *sup)
{
  _engineSupervisor = sup;
  _engineSupervisorSet = true;
}


void CubeBleClient::StartScanUponConnection()
{
  DEV_ASSERT(!_inited, "CubeBleClient.StartScanningUponConnection.AlreadyInited");
  _scanUponConnection = true;
}


void CubeBleClient::SetScanDuration(const float duration_sec)
{
  _scanDuration_sec = duration_sec;
}


void CubeBleClient::StartScanning()
{
  _scanning = true;
  _scanUntil_sec = _scanDuration_sec + _engineSupervisor->getTime();
}


void CubeBleClient::StopScanning()
{
  _scanUntil_sec = _engineSupervisor->getTime();
}


bool CubeBleClient::SendMessageToLightCube(const BleFactoryId& factoryID, const MessageEngineToCube& msg)
{
  DEV_ASSERT(IsConnectedToCube(factoryID), "CubeBleClient.SendMessageToLightCube.CubeNotConnected");
  
  const int channel = GetEmitterChannel(factoryID);
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
  
  // Grab an available receiver for this cube:
  DEV_ASSERT(_cubeReceiverMap.find(factoryId) == _cubeReceiverMap.end(), "CubeBleClient.ConnectToCube.ReceiverAlreadyAssigned");
  for (auto* rec : _receivers) {
    // Is this receiver already in use by another cube?
    const auto it = std::find_if(_cubeReceiverMap.begin(), _cubeReceiverMap.end(),
                                 [rec](const std::pair<BleFactoryId, webots::Receiver*>& mapItem) {
                                   return rec == mapItem.second;
                                 });
    if (it == _cubeReceiverMap.end()) {
      // This receiver is free
      rec->setChannel(GetReceiverChannel(factoryId));
      rec->enable(CUBE_TIME_STEP_MS);
      _cubeReceiverMap[factoryId] = rec;
      break;
    }
  }
  
  DEV_ASSERT_MSG(_cubeReceiverMap.find(factoryId) != _cubeReceiverMap.end(),
                 "CubeBleClient.ConnectToCube.NoReceiverAssigned",
                 "Could not find a free receiver for cube with factory ID %s. Connected to too many cubes?",
                 factoryId.c_str());
  
  // Mark as connected and immediately call connected callback
  _connectedToCube[factoryId] = true;
  for (const auto& callback : _cubeConnectionCallbacks) {
    callback(factoryId, true);
  }
  return true;
}


bool CubeBleClient::DisconnectFromCube(const BleFactoryId& factoryId)
{
  DEV_ASSERT(IsConnectedToCube(factoryId), "CubeBleClient.ConnectToCube.NotConnected");
  
  // Disable and remove the receiver associated with this cube;
  const auto receiverIt = _cubeReceiverMap.find(factoryId);
  if (receiverIt != _cubeReceiverMap.end()) {
    auto* receiver = receiverIt->second;
    receiver->disable();
    _cubeReceiverMap.erase(receiverIt);
  }
  
  // Mark as disconnected and immediately call disconnected callback
  _connectedToCube[factoryId] = false;
  for (const auto& callback : _cubeConnectionCallbacks) {
    callback(factoryId, false);
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

  
bool CubeBleClient::Init()
{
  if (_scanUponConnection) {
    _scanUntil_sec = _scanDuration_sec + _engineSupervisor->getTime();
    _scanning = true;
  }
  
  _inited = true;
  return true;
}

  
bool CubeBleClient::Update()
{
  DEV_ASSERT(_inited, "CubeBleClient.Update.NotInited");
  
  // Look for discovery/advertising messages:
  while (_discoveryReceiver->getQueueLength() > 0) {
    // Shove the data into a MessageCubeToEngine and call callbacks.
    MessageCubeToEngine cubeMessage((uint8_t *) _discoveryReceiver->getData(), (size_t) _discoveryReceiver->getDataSize());
    const auto sigStrength = _discoveryReceiver->getSignalStrength();
    _discoveryReceiver->nextPacket();
    if (cubeMessage.GetTag() == MessageCubeToEngineTag::available) {
      // Received an advertisement message
      ExternalInterface::ObjectAvailable msg(cubeMessage.Get_available());
      
      // Webots signal strength is 1/r^2 with r = distance in meters.
      // Therefore typical webots signal strength values are in (0, ~150) or so.
      // Typical RSSI values for physical cubes range from -100 to -30 or so.
      // So map (0, 150) -> (-100, -30).
      const double rssiDbl = -100.0 + (sigStrength / 150.0) * 70.0;
      msg.rssi = Util::numeric_cast_clamped<decltype(msg.rssi)>(rssiDbl);
      
      // Call the appropriate callbacks with the modified message, but only if the state is 'disconnected'
      // and we are actively scanning
      if (_scanning && !IsConnectedToCube(msg.factory_id)) {
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
  for (const auto& mapEntry : _cubeReceiverMap) {
    const auto& factoryId = mapEntry.first;
    auto* receiver = mapEntry.second;
    while (receiver->getQueueLength() > 0) {
      MessageCubeToEngine cubeMessage((uint8_t *) receiver->getData(), (size_t) receiver->getDataSize());
      receiver->nextPacket();
      // Received a light cube message. Call the registered callbacks.
      for (const auto& callback : _cubeMessageCallbacks) {
        callback(factoryId, cubeMessage);
      }
    }
  }
  
  // Check for the end of the scanning period
  if (_scanning && _engineSupervisor->getTime() >= _scanUntil_sec) {
    for (const auto& callback : _scanFinishedCallbacks) {
      callback();
    }
    _scanning = false;
  }
  
  return true;
}


} // namespace Cozmo
} // namespace Anki
