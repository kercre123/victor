/**
 * File: blockFilter.h
 *
 * Author: Molly Jameson
 * Date:   2/18/2016
 *
 * Description: blockFilter is a serializable set of factoryIds.
 *                So Cozmo only uses certain blocks.
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Cozmo_Basestation_Blocks_BlockFilter_H__
#define __Cozmo_Basestation_Blocks_BlockFilter_H__

#include "anki/cozmo/basestation/activeCube.h"
#include "clad/types/activeObjectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace Anki {
namespace Cozmo {

class IExternalInterface;
template <typename Type>
class AnkiEvent;
class Robot;

namespace ExternalInterface {
  class MessageGameToEngine;
  class MessageEngineToGame;
}
  
class BlockFilter
{
public: 
  BlockFilter(Robot* inRobot, IExternalInterface* externalInterface);

  void Init(const std::string &path);

  void Update();
  
private:
  
  struct ObjectInfo
  {
    ObjectInfo()
    {
      Reset();
    }
    
    void Reset()
    {
      factoryID = ActiveObject::InvalidFactoryID;
      objectType = ObjectType::Invalid;
    }
    
    FactoryID 			factoryID;
    ObjectType 			objectType;
  };
  
  static constexpr double kConnectivityCheckDelay = 1.0f; // How often do we check for connectivity changes in seconds
  static constexpr double kMaxWaitForPooledObjects = 5.0f; // How long do we wait for the discovered objects to connect before looking for another one of the same type
  
  static constexpr uint8_t kMaxRSSI = 150;
  
  // These are the type of objects we care about
  static constexpr std::array<ObjectType, 4> kObjectTypes = {{
    ObjectType::Block_LIGHTCUBE1,
    ObjectType::Block_LIGHTCUBE2,
    ObjectType::Block_LIGHTCUBE3,
    ObjectType::Charger_Basic
  }};
  
  void UpdateDiscovering();
  void UpdateConnecting();
  
  bool AddObjectToPersistentPool(FactoryID factoryID, ObjectType objectType);
  bool RemoveObjectFromPersistentPool(FactoryID factoryID);
  void CopyPersistentPoolToRuntimePool();
  
  void Load();
  void Save() const;
  
  bool IsTypePooled(ObjectType type) const;
  
  void ConnectToObjects();
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  void SendBlockPoolData() const;
  
  using ObjectInfoArray = std::array<ObjectInfo, (size_t)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS>;
  using ObjectInfoMap = std::map<ObjectType, ObjectInfo>;
  
  Robot*              _robot;
  ObjectInfoArray     _persistentPool;						// The list of objects from the file
  ObjectInfoArray     _runtimePool;								// The current session pool list
  ObjectInfoMap       _discoveryPool;             // Selected set of objects until the discovery phase is over
  std::string         _path;											// Path where we are saving/loading the list of objects
  double              _maxDiscoveryTime;          // Time we'll spend looking for objects before pooling
  double              _enabledTime;								// Time when the block pool was enabled
  double              _discoveredCompletedTime;   // Time when the last discovery period was done
  double              _lastConnectivityCheckTime; // Used to check for connectivity every once in a while
  bool                _enabled;                   // True if the automatic pool is enabled
  
  std::vector<Signal::SmartHandle> _signalHandles;
  IExternalInterface* _externalInterface;

};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_Blocks_BlockFilter_H__) */
