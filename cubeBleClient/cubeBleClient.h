/**
 * File: cubeBleClient.h
 *
 * Author: Matt Michini
 * Created: 12/1/2017
 *
 * Description:
 *               Defines interface to BLE-connected real or simulated cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Victor_CubeBleClient_H__
#define __Victor_CubeBleClient_H__

#include "coretech/common/shared/types.h"

#include <vector>
#include <functional>

// Forward declaration
namespace webots {
  class Supervisor;
}

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
  struct ObjectAvailable;
}
class MessageEngineToCube;
class MessageCubeToEngine;

// Alias for BLE factory ID (TODO: should be defined in CLAD)
using BleFactoryId = std::string;
  
class CubeBleClient
{
public:
  CubeBleClient();
  ~CubeBleClient();
  
  bool Init();
  
  bool Update();

#ifdef SIMULATOR
  // Assign Webots supervisor
  // Webots processes must do this before creating CubeBleClient for the first time.
  // Unit test processes must call SetSupervisor(nullptr) to run without a supervisor.
  static void SetSupervisor(webots::Supervisor *sup);
#endif

  using ObjectAvailableCallback   = std::function<void(const ExternalInterface::ObjectAvailable&)>;
  using CubeMessageCallback       = std::function<void(const BleFactoryId&, const MessageCubeToEngine&)>;
  using CubeConnectionCallback    = std::function<void(const BleFactoryId&, const bool connected)>;
  using ScanFinishedCallback      = std::function<void(void)>;
  
  void RegisterObjectAvailableCallback(const ObjectAvailableCallback& callback) {
    _objectAvailableCallbacks.push_back(callback);
  }
  void RegisterCubeMessageCallback(const CubeMessageCallback& callback) {
    _cubeMessageCallbacks.push_back(callback);
  }
  void RegisterCubeConnectionCallback(const CubeConnectionCallback& callback) {
    _cubeConnectionCallbacks.push_back(callback);
  }
  void RegisterScanFinishedCallback(const ScanFinishedCallback& callback) {
    _scanFinishedCallbacks.push_back(callback);
  }
  
  // Tells the bleClient to start scanning for cubes immediately upon connection
  // to the anki bluetooth daemon
  void StartScanUponConnection();
  
  void SetScanDuration(const float duration_sec);
  
  void StartScanning();
  
  void StopScanning();
  
  // Send a message to the specified light cube. Returns true on success.
  bool SendMessageToLightCube(const BleFactoryId&, const MessageEngineToCube&);
  
  // Request to connect to an advertising cube. Returns true on success.
  bool ConnectToCube(const BleFactoryId&);
  
  // Request to disconnect from a connected cube. Returns true on success.
  bool DisconnectFromCube(const BleFactoryId&);
  
  // Is this cube connected?
  bool IsConnectedToCube(const BleFactoryId&);
  
private:
  
  // callbacks for advertisement messages:
  std::vector<ObjectAvailableCallback> _objectAvailableCallbacks;
  
  // callbacks for raw light cube messages:
  std::vector<CubeMessageCallback> _cubeMessageCallbacks;
  
  // callbacks for when a cube is connected/disconnected:
  std::vector<CubeConnectionCallback> _cubeConnectionCallbacks;
  
  // callbacks for when scanning for cubes has completed:
  std::vector<ScanFinishedCallback> _scanFinishedCallbacks;
  
  bool _inited = false;
  
}; // class CubeBleClient
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Victor_CubeBleClient_H__
