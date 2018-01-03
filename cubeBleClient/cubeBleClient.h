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

#include "util/singleton/dynamicSingleton.h"

#include <vector>
#include <functional>

// Forward declaration
namespace webots {
  class Supervisor;
}

namespace Anki {
namespace Cozmo {
  
struct ObjectAvailable;
namespace BlockMessages {
  class LightCubeMessage;
}

// Alias for BLE factory ID (TODO: should be defined in CLAD)
using BleFactoryId = int;
  
class CubeBleClient : private Util::DynamicSingleton<CubeBleClient>
{
  ANKIUTIL_FRIEND_SINGLETON(CubeBleClient);
  
public:

  // Method to fetch singleton instance.
  static CubeBleClient* GetInstance();
  
  Result Update();

#ifdef SIMULATOR
  // Assign Webots supervisor
  // Webots processes must do this before creating CubeBleClient for the first time.
  // Unit test processes must call SetSupervisor(nullptr) to run without a supervisor.
  static void SetSupervisor(webots::Supervisor *sup);
#endif

  using ObjectAvailableCallback   = std::function<void(const ObjectAvailable&)>;
  using LightCubeMessageCallback  = std::function<void(const BleFactoryId&, const BlockMessages::LightCubeMessage&)>;
  using CubeConnectedCallback     = std::function<void(const BleFactoryId&)>;
  using CubeDisconnectedCallback  = std::function<void(const BleFactoryId&)>;
  
  void RegisterObjectAvailableCallback(const ObjectAvailableCallback& callback) {
    _objectAvailableCallbacks.push_back(callback);
  }
  void RegisterLightCubeMessageCallback(const LightCubeMessageCallback& callback) {
    _lightCubeMessageCallbacks.push_back(callback);
  }

  void RegisterCubeConnectedCallback(const CubeConnectedCallback& callback) {
    _cubeConnectedCallbacks.push_back(callback);
  }

  void RegisterCubeDisconnectedCallback(const CubeDisconnectedCallback& callback) {
    _cubeDisconnectedCallbacks.push_back(callback);
  }
  
  // Send a message to the specified light cube. Returns true on success.
  bool SendMessageToLightCube(const BleFactoryId&, const BlockMessages::LightCubeMessage&);
  
  // Request to connect to an advertising cube. Returns true on success.
  bool ConnectToCube(const BleFactoryId&);
  
  // Request to disconnect from a connected cube. Returns true on success.
  bool DisconnectFromCube(const BleFactoryId&);
  
  // Is this cube connected?
  bool IsConnectedToCube(const BleFactoryId&);
  
private:
  
  // private constructor:
  CubeBleClient();
  
  // callbacks for advertisement messages:
  std::vector<ObjectAvailableCallback> _objectAvailableCallbacks;
  
  // callbacks for raw light cube messages:
  std::vector<LightCubeMessageCallback> _lightCubeMessageCallbacks;
  
  // callbacks for when a cube is connected:
  std::vector<CubeConnectedCallback> _cubeConnectedCallbacks;
  
  // callbacks for when a cube is disconnected:
  std::vector<CubeDisconnectedCallback> _cubeDisconnectedCallbacks;
  
}; // class CubeBleClient
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Victor_CubeBleClient_H__
