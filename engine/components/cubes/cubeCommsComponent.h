/**
 * File: cubeCommsComponent.h
 *
 * Author: Matt Michini
 * Created: 11/29/2017
 *
 * Description: Component for managing communications with light cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_CubeCommsComponent_H__
#define __Engine_Components_CubeCommsComponent_H__

#include "cubeBleClient/cubeBleClient.h" // alias BleFactoryId (should be moved to CLAD)

#include "engine/cozmoObservableObject.h" // alias ActiveID

#include "util/entityComponent/entity.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h" // Signal::SmartHandle

#include <unordered_map>

namespace Anki {
namespace Vector {
  
// forware decl:
class Robot;
template <typename Type>
class AnkiEvent;
struct CubeLightKeyframeChunk;
struct CubeLights;
struct CubeLightSequence;
class MessageCubeToEngine;
class MessageEngineToCube;
  
namespace ExternalInterface {
  class MessageGameToEngine;
  class MessageEngineToGame;
  struct ObjectAvailable;
}
namespace external_interface {
  class GatewayWrapper;
}
  
class CubeCommsComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  
  using ConnectionCallback = std::function<void(bool success)>;
  
  // Playpen test needs to be able to start and stop cube scans
  friend class BehaviorPlaypenTest;
  
  CubeCommsComponent();
  ~CubeCommsComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // Requests a connection to a cube. Returns true if the request
  // was successful (does not mean it is connected to the cube yet).
  // The provided callback gets called once the connection process
  // either succeeds or fails.
  //
  // Note: If this function returns false, the callback will _not_
  // be called.
  bool RequestConnectToCube(const ConnectionCallback& connectedCallback = {});
  
  // Requests a disconnection from the currently connected cube.
  // The connection will be held for gracePeriod_sec before actually
  // disconnecting. The provided callback is called once the cube
  // is fully disconnected.
  //
  // Note: If this function returns false, the callback will _not_
  // be called.
  bool RequestDisconnectFromCube(const float gracePeriod_sec, const ConnectionCallback& disconnectedCallback = {});
  
  // Set the robot's preferred cube and save it to disk. The robot
  // will always attempt to connect to this cube if it is available.
  void SetPreferredCube(const BleFactoryId& factoryId);
  
  // Return the factory ID of the preferred cube, or an empty string if there is none.
  BleFactoryId GetPreferredCube() const { return _preferredCubeFactoryId; }
  
  // 'Forget' the robot's preferred cube. This will cause the robot to
  // connect to the cube with the highest RSSI (signal strength) next
  // time a connection is requested. Saves this preference to disk.
  void ForgetPreferredCube();
  
  // Send CubeLights message to the currently connected cube
  bool SendCubeLights(const CubeLights& cubeLights);
  
  // Returns the ActiveID of the currently-connected cube, or
  // ObservableObject::InvalidActiveID if there is no connected cube
  ActiveID GetConnectedCubeActiveId() const;
  
  // Returns the FactoryId of the currently-connected cube, or empty string if there is no connected cube
  BleFactoryId GetConnectedCubeFactoryId() const;
  
  // Set whether or not to broadcast ObjectAvailable messages to game
  void SetBroadcastObjectAvailable(const bool enable = true) { _broadcastObjectAvailableMsg = enable; }
  
  // Return the BLE client's current cube connection state
  CubeConnectionState GetCubeConnectionState() const { return _cubeBleClient->GetCubeConnectionState(); }
  
  // True if we are _fully_ connected to a cube over BLE
  bool IsConnectedToCube() const { return GetCubeConnectionState() == CubeConnectionState::Connected; }
  
  // True if a cube disconnection has been scheduled
  bool IsDisconnectScheduled() const { return _disconnectFromCubeTime_sec > 0.f; }

protected:
  
  // Clients should not need access to these protected methods, except
  // in the case of e.g. factory test.
  
  // Tells the BLE client to start a scan for cubes. If autoConnectAfterScan is
  // true, attempt to connect to a cube immediately after scanning is complete.
  bool StartScanningForCubes(const bool autoConnectAfterScan = true);

  // Tells the BLE client to stop scanning for cubes
  void StopScanningForCubes();
  
private:
  
  // Returns the factory ID of the cube we are either currently
  // connected to, or pending connection or disconnection to.
  // It is an empty string if there is no current cube.
  BleFactoryId GetCurrentCube() const { return _cubeBleClient->GetCurrentCube(); }
  
  // Send a message to currently connected light cube
  bool SendCubeMessage(const MessageEngineToCube& msg);
  
  // From the given CubeLights struct, generates a set of cube light messages
  // to be sent over BLE to a cube.
  void GenerateCubeLightMessages(const CubeLights& cubeLights,
                                 CubeLightSequence& cubeLightSequence,
                                 std::vector<CubeLightKeyframeChunk>& cubeLightKeyframeChunks);
  
  // Game to engine event handlers
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  // App to engine event handlers
  void HandleAppEvents(const AnkiEvent<external_interface::GatewayWrapper>& event);
  
  // Handlers for messages from CubeBleClient:
  
  // Handler for ObjectAvailable advertisement message
  void HandleObjectAvailable(const ExternalInterface::ObjectAvailable& msg);
  
  // Handler for messages from light cubes
  void HandleCubeMessage(const BleFactoryId& factoryId, const MessageCubeToEngine& msg);
  
  // Handler for when light cube BLE connection is established/unestablished
  void HandleConnectionStateChange(const BleFactoryId& factoryId, const bool connected);
  
  // Handler for when light cube BLE connection is established
  void OnCubeConnected(const BleFactoryId& factoryId);

  // Handler for when light cube BLE connection is un-established
  void OnCubeDisconnected(const BleFactoryId& factoryId);
  
  // Handler for when scanning for cubes has finished
  void HandleScanForCubesFinished();
  
  // Handler for when a connection attempt fails
  void HandleConnectionFailed(const BleFactoryId& factoryId);
  
  // Get the ActiveID to assign to the next object added to the available cubes
  // list (just to make sure each object gets a unique ActiveID)
  ActiveID GetNextActiveId() const;
  
  // Find a cube's ActiveID from its factoryId. Returns ObservableObject::InvalidActiveID if not found
  ActiveID GetActiveId(const BleFactoryId& factoryId) const;
  
  // Read the preferred cube from the persistent file. If the file is not found
  // or could not be parsed, then create a file with no preferred cube.
  void ReadPreferredCubeFromFile();
  
  void SubscribeToWebViz();
  
  void SendDataToWebViz();
 
  Robot* _robot = nullptr;
  
  std::unique_ptr<CubeBleClient> _cubeBleClient;
  
  // Handles for grabbing GameToEngine messages and web viz
  std::vector<Signal::SmartHandle> _signalHandles;
  
  std::vector<ConnectionCallback> _connectedCallbacks;
  
  std::vector<ConnectionCallback> _disconnectedCallbacks;
  
  // When true, a cube scan will be initiated once the pending
  // cube disconnection completes.
  bool _startScanWhenUnconnected = false;
  
  // If true, immediately attempt to connect to a cube after scanning completes.
  bool _connectAfterScan = true;
  
  // When to disconnect from the currently connected cube
  // (used for cube disconnection "grace period")
  float _disconnectFromCubeTime_sec = -1.f;
  
  // Whether or not to broadcast incoming ObjectAvailable messages to game
  bool _broadcastObjectAvailableMsg = false;
  
  // The list of cubes that we have heard from while scanning for
  // cubes. It is a map of FactoryID to last RSSI (signal strength).
  std::map<BleFactoryId, int> _cubeScanResults;
  
  // Map of factoryID to ActiveID for quicker lookup based on factoryID.
  // This map is persistent and items are never removed from it. This is
  // to ensure that objects with the same factory ID get assigned the same
  // active ID even after they've disappeared and reconnected.
  std::unordered_map<BleFactoryId, ActiveID> _factoryIdToActiveIdMap;
  
  // cached preferred block to connect to
  BleFactoryId _preferredCubeFactoryId = "";
  
  // Next time we should send data to webviz
  float _nextSendWebVizDataTime_sec = 0.f;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_CubeCommsComponent_H__
