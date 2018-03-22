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

#include "engine/cozmoObservableObject.h" // alias ActiveID

#include "cubeBleClient/cubeBleClient.h" // alias BleFactoryId (should be moved to CLAD)
#include "util/entityComponent/entity.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h" // Signal::SmartHandle

#include <unordered_map>

namespace Anki {
namespace Cozmo {
  
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
  
class CubeCommsComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  CubeCommsComponent();
  ~CubeCommsComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CubeAccel);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////

  void Update();
  
  // Enter or leave discovery mode for the requested duration (uses default if none specified).
  // In discover mode, component will listen for advertising cubes and connect to the 'best' ones.
  // If any cubes are connected, they will be disconnected.
  void EnableDiscovery(const bool enable = true, const float discoveryTime_sec = 0.f);
  
  // Interface for other components to send messages to light cubes by ActiveId
  bool SendCubeMessage(const ActiveID& activeId, const MessageEngineToCube& msg);
  
  // Send CubeLights message to the specified cube
  bool SendCubeLights(const ActiveID& activeId, const CubeLights& cubeLights);
  
  // Sends current available cube list to game
  void SendBlockPoolData() const;
  
  // Game to engine event handlers
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
private:
  
  Robot* _robot = nullptr;
  
  CubeBleClient* _cubeBleClient;
  
  // Handles for grabbing GameToEngine messages
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // From the given CubeLights struct, generates a set of cube light messages
  // to be sent over BLE to a cube.
  void GenerateCubeLightMessages(const CubeLights& cubeLights,
                                 CubeLightSequence& cubeLightSequence,
                                 std::vector<CubeLightKeyframeChunk>& cubeLightKeyframeChunks);
  
  // Handlers for messages from CubeBleClient:
  
  // Handler for ObjectAvailable advertisement message
  void HandleObjectAvailable(const ExternalInterface::ObjectAvailable& msg);
  
  // Handler for messages from light cubes
  void HandleCubeMessage(const BleFactoryId& factoryId, const MessageCubeToEngine& msg);
  
  // Handler for when light cube BLE connection is established/unestablished
  void HandleConnectionStateChange(const BleFactoryId& factoryId, const bool connected);
  
  // Get the ActiveID to assign to the next object added to the available cubes
  // list (just to make sure each object gets a unique ActiveID)
  ActiveID GetNextActiveId();
  
  // If discovering, then we are listening for any advertising cubes and
  // selecting the best ones to connect to.
  bool _discovering = false;
  float _discoveringEndTime_sec = 0.f;
  
  // Next time we're supposed to check for disconnections
  float _nextDisconnectCheckTime_sec = 0.f;
  
  // Whether or not to broadcast incoming ObjectAvailable messages to game
  bool _broadcastObjectAvailableMsg = false;
  
  struct CubeInfo
  {
    BleFactoryId factoryId;
    ObjectType objectType;
    float lastHeardTime_sec;
    int lastRssi;
    bool connected;
  };
  
  // The main list of cubes we know about:
  std::map<ActiveID, CubeInfo> _availableCubes;
  
  // Map of factoryID to ActiveID for quicker lookup based on factoryID.
  // This map is persistent and items are never removed from it. This is
  // to ensure that objects with the same factory ID get assigned the same
  // active ID even after they've disappeared and reconnected.
  std::unordered_map<BleFactoryId, ActiveID> _factoryIdToActiveIdMap;
  
  // AddCubeToList() generates a new activeId and adds the cube to the _availableCubes list if it's not in there already.
  // Returns true if it was added, false if already there.
  bool AddCubeToList(const CubeInfo& cube);
  
  // Remove cube from the list based on BleFactoryId. Returns true if cube was successfully removed.
  bool RemoveCubeFromList(const BleFactoryId& factoryId);
  
  // Find a cube in the list by ActiveID and return a pointer to it. Returns nullptr if not found.
  CubeInfo* GetCubeByActiveId(const ActiveID& activeId);
  
  // Find a cube in the list by factoryId and return a pointer to it. Returns nullptr if not found.
  CubeInfo* GetCubeByFactoryId(const BleFactoryId& factoryId);
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_CubeCommsComponent_H__
