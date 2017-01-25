/**
 * File: blockFilter.cpp
 *
 * Author: Molly Jameson
 * Date:   2/18/2016
 *
 * Description: blockFilter is a serializable set of factoryIds.
 *                So Cozmo only uses certain blocks.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/blocks/blockFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "util/logging/logging.h"

#include <util/helpers/includeFstream.h>

#include <sys/stat.h>

// WORKAROUND: For some reason objects in slot 0 display lights incorrectly so don't use it.
//             The real fix belongs in robot body firmware.
#define DONT_USE_SLOT0 0


namespace Anki {
namespace Cozmo {
  
constexpr std::array<ObjectType, 4> BlockFilter::kObjectTypes;

BlockFilter::BlockFilter(Robot* inRobot, IExternalInterface* externalInterface)
  : _robot(inRobot)
  , _path()
  , _maxDiscoveryTime(0)
  , _enabledTime(0)
  , _discoveredCompletedTime(0)
  , _lastConnectivityCheckTime(0)
  , _enabled(false)
  , _externalInterface(externalInterface)
{
  if (_externalInterface != nullptr) {
    auto callback = std::bind(&BlockFilter::HandleGameEvents, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolResetMessage, callback));
  }
}

void BlockFilter::Init(const std::string &path)
{
    _path = path;

    // Load the existing list of objects if there is one. Then copy them to the runtime pool and connect to all the objects
    PRINT_CH_INFO("BlockPool", "BlockFilter.Init", "Loading from file %s", path.c_str());
    Load();
  
    CopyPersistentPoolToRuntimePool();
    ConnectToObjects();
}
  
void BlockFilter::Update()
{
  if (!_enabled)
  {
    return;
  }
  
  // Check connectivity only every once in a while
  double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((_lastConnectivityCheckTime > 0) && (currentTime < (_lastConnectivityCheckTime + kConnectivityCheckDelay))) {
    return;
  }
  
  UpdateDiscovering();
  UpdateConnecting();
  
  _lastConnectivityCheckTime = currentTime;
}

void BlockFilter::UpdateDiscovering()
{
  // For every object type we care about that is not in the pool already, save the closest available.
  for (auto objectType : kObjectTypes)
  {
    if (!IsTypePooled(objectType))
    {
      // Look for the closest object of the type in a given signal strength
      PRINT_CH_INFO("BlockPool", "BlockFilter.UpdateDiscovering", "Looking for objects of type %s with RSSI < %d", EnumToString(objectType), kMaxRSSI);
      FactoryID closestObject = _robot->GetClosestDiscoveredObjectsOfType(objectType, kMaxRSSI);
      if (closestObject != ActiveObject::InvalidFactoryID)
      {
        PRINT_CH_INFO("BlockPool", "BlockFilter.UpdateDiscovering", "Discovered closer object 0x%x", closestObject);
        ObjectInfo &objectInfo = _discoveryPool[objectType];
        objectInfo.factoryID = closestObject;
        objectInfo.objectType = objectType;
      }
    }
  }

  // If the discovery phase is over, then connect to the objects we found
  double time = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((time >= (_enabledTime + _maxDiscoveryTime)) && (_discoveryPool.size() > 0))
  {
    PRINT_CH_INFO("BlockPool", "BlockFilter.UpdateDiscovering", "Connecting to discovered objects");
    // Save the new pool and connect to the discovered objects
    for (const auto& kvp : _discoveryPool)
    {
      AddObjectToPersistentPool(kvp.second.factoryID, kvp.second.objectType);
    }
    _discoveryPool.clear();
    CopyPersistentPoolToRuntimePool();
    ConnectToObjects();
    SendBlockPoolData();
    _discoveredCompletedTime = time;
  }
}

void BlockFilter::UpdateConnecting()
{
  // Check if any of the objects in the runtime pool is still not connected after the initial waiting period
  if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() < _discoveredCompletedTime + kMaxWaitForPooledObjects)
  {
    return;
  }
  
  for (ObjectInfo& objectInfo : _runtimePool)
  {
    if ((objectInfo.factoryID != ActiveObject::InvalidFactoryID) && !_robot->IsConnectedToObject(objectInfo.factoryID))
    {
      PRINT_CH_INFO("BlockPool", "BlockFilter.UpdateConnecting", "Looking for a replacement for object 0x%x of type %s", objectInfo.factoryID, EnumToString(objectInfo.objectType));
      FactoryID closestObject = _robot->GetClosestDiscoveredObjectsOfType(objectInfo.objectType, kMaxRSSI);
      if ((closestObject != ActiveObject::InvalidFactoryID) && (closestObject != objectInfo.factoryID))
      {
        PRINT_CH_INFO("BlockPool", "BlockFilter.UpdateConnecting", "Found replacement object 0x%x", closestObject);
        objectInfo.factoryID = closestObject;
        ConnectToObjects();
      }
    }
  }
}

bool BlockFilter::AddObjectToPersistentPool(FactoryID factoryID, ObjectType objectType)
{
  // Find an available slot
  auto startIter = _persistentPool.begin();

  if (DONT_USE_SLOT0) {
    ++startIter;
  }
  
  auto it = _persistentPool.end();
  for(auto iter = startIter; iter != _persistentPool.end(); ++iter)
  {
    if(iter->factoryID == factoryID || iter->objectType == objectType)
    {
      PRINT_NAMED_ERROR("BlockFilter.AddObjectToPersistentPool",
                        "Object with factory ID 0x%x and/or type %s already in _persistentPool",
                        factoryID,
                        EnumToString(objectType));
      return false;
    }
    
    if(it == _persistentPool.end() && iter->factoryID == ActiveObject::InvalidFactoryID)
    {
      it = iter;
    }
  }
  
  if (it != _persistentPool.end()) {
    PRINT_CH_INFO("BlockPool", "BlockFilter.AddObjectToPersistentPool", "Adding object 0x%x of type %s to the persistent pool", factoryID, EnumToString(objectType));

    it->Reset();
    it->factoryID = factoryID;
    it->objectType = objectType;
    
    Save();
    return true;
  } else {
    PRINT_NAMED_ERROR("BlockFilter.AddFactoryId", "There is no available slot to add factory ID %#08x,", factoryID);
    return false;
  }
}

bool BlockFilter::RemoveObjectFromPersistentPool(FactoryID factoryID)
{
  ObjectInfoArray::iterator it = std::find_if(_persistentPool.begin(), _persistentPool.end(), [factoryID](ObjectInfo& objectInfo) {
    return objectInfo.factoryID == factoryID;
  });
  
  if (it != _persistentPool.end()) {
    PRINT_CH_INFO("BlockPool", "BlockFilter.RemoveObjectFromPersistentPool", "Removing object 0x%x of type %s from persistent pool", factoryID, EnumToString(it->objectType));

    it->Reset();
    
    Save();
    return true;
  } else {
    PRINT_CH_INFO("BlockPool", "BlockFilter.RemoveObjectFromPersistentPool", "Object 0x%x not found in persistent pool", factoryID);
    return false;
  }
}

void BlockFilter::CopyPersistentPoolToRuntimePool()
{
  std::copy(_persistentPool.begin(), _persistentPool.end(), _runtimePool.begin());
}

void BlockFilter::Load()
{
  std::ifstream inputFileStream;
  inputFileStream.open(_path);
  if (!inputFileStream.good())
  {
    inputFileStream.close();
    return;
  }
  
  int lineIndex = 0;
  
  if (DONT_USE_SLOT0) {
    lineIndex = 1;
  }
  
  std::string line;
  while (std::getline(inputFileStream, line))
  {
    if (line.length() == 0)
    {
      continue;
    }
    
    if (lineIndex >= _persistentPool.size())
    {
      PRINT_NAMED_ERROR("BlockFilter.Load", "Found more than %d lines in the file. They will be ignored", (int)_persistentPool.size());
      break;
    }
    
    if (line.compare(0,2,"0x") == 0)
    {
      try
      {
        std::size_t indexOfComma = 0;
        
        FactoryID factoryID = (FactoryID)std::stoul(line, &indexOfComma, 16);

        // For compatibility with the old blockpool file format we handle the case where the line doesn't have
        // a type. In that case, we discard it.
        if (indexOfComma < line.length()) {
          ObjectInfo& objectInfo = _persistentPool[lineIndex];
          objectInfo.factoryID = factoryID;
          objectInfo.objectType = (ObjectType)std::stoul(line.substr(indexOfComma + 1));

          ++lineIndex;
          PRINT_CH_INFO("BlockPool", "BlockFilter.Load", "%#08x,%d", objectInfo.factoryID, objectInfo.objectType);
        }
      }
      catch (std::exception e)
      {
        PRINT_NAMED_DEBUG("BlockFilter.Load.ParseError", "%s", line.c_str());
      }
    }
  }
  
  inputFileStream.close();
}
  
void BlockFilter::Save() const
{
  if (_path.empty())
  {
    return;
  }
  
  // Find if there is any block connnected
  ObjectInfoArray::const_iterator it = std::find_if(_persistentPool.begin(), _persistentPool.end(), [](const ObjectInfo& object) {
    return object.factoryID != ActiveObject::InvalidFactoryID;
  });
  
  if (it == std::end(_persistentPool))
  {
    // list is empty, delete the file if is exists
    struct stat buf;
    if (stat(_path.c_str(), &buf) == 0)
    {
      remove(_path.c_str());
    }
  }

  std::ofstream outputFileStream;
  outputFileStream.exceptions(~std::ofstream::goodbit);
  try
  {
    outputFileStream.open(_path);
    for (const ObjectInfo& objectInfo : _persistentPool)
    {
      if (objectInfo.factoryID != ActiveObject::InvalidFactoryID)
      {
        outputFileStream << "0x";
        outputFileStream << std::hex << objectInfo.factoryID;
        outputFileStream << ",";
        outputFileStream << std::to_string((int)objectInfo.objectType);
        outputFileStream << "\n";
        PRINT_CH_INFO("BlockPool", "BlockFilter.Save", "%#08x,%d", objectInfo.factoryID, objectInfo.objectType);
      }
    }
    outputFileStream.close();
  }
  catch (std::ofstream::failure e)
  {
    PRINT_NAMED_ERROR("BlockFilter.Save.Error", "%s", e.std::exception::what());
  }
}

bool BlockFilter::IsTypePooled(ObjectType type) const
{
  for (const ObjectInfo& objectInfo : _persistentPool)
  {
    if (objectInfo.objectType == type)
    {
      return true;
    }
  }
  
  return false;
}

void BlockFilter::ConnectToObjects()
{
  FactoryIDArray factoryIDs;

  // Connect to the current list of objects
  for (int i = 0; i < _runtimePool.size(); ++i)
  {
    factoryIDs[i] = _runtimePool[i].factoryID;
  }
  
  _robot->ConnectToObjects(factoryIDs);
}
  
void BlockFilter::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    // Debug menu tab just opened, asking for the current state of things
    case ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage:
    {
      SendBlockPoolData();
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage:
    {
      const Anki::Cozmo::ExternalInterface::BlockPoolEnabledMessage& msg = event.GetData().Get_BlockPoolEnabledMessage();
      
      if (msg.enabled)
      {
        PRINT_CH_INFO("BlockPool", "BlockFilter.HandleGameEvents", "Enabling automatic block pool with a discovery time of %f seconds", msg.discoveryTimeSecs);
      }
      else
      {
        PRINT_CH_INFO("BlockPool", "BlockFilter.HandleGameEvents", "Disabling automatic block pool");
      }

      _maxDiscoveryTime = msg.discoveryTimeSecs;
      _enabled = msg.enabled;
      _enabledTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _discoveredCompletedTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage:
    {
      const Anki::Cozmo::ExternalInterface::BlockSelectedMessage& msg = event.GetData().Get_BlockSelectedMessage();
      if (msg.selected)
      {
        if (AddObjectToPersistentPool(msg.factoryId, msg.objectType))
        {
          CopyPersistentPoolToRuntimePool();
          ConnectToObjects();
          SendBlockPoolData();
        }
      }
      else
      {
        if (RemoveObjectFromPersistentPool(msg.factoryId))
        {
          // Object has been removed from the persistent pool, disconnect from it
          CopyPersistentPoolToRuntimePool();
          ConnectToObjects();
          SendBlockPoolData();
        }
      }
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::BlockPoolResetMessage:
    {
      PRINT_CH_INFO("BlockPool", "BlockFilter.HandleGameEvents", "Reseting the blockpool");
      
      // Reset the runtime and persistent pools, disconnect from all the objects, and then save to disk
      std::for_each(_runtimePool.begin(), _runtimePool.end(), [](ObjectInfo& objectInfo) {
        objectInfo.Reset();
      });

      std::for_each(_persistentPool.begin(), _persistentPool.end(), [](ObjectInfo& objectInfo) {
        objectInfo.Reset();
      });
      
      ConnectToObjects();
      
      Save();
      
      // Reset the connectivity check time so we give some time to the cubes to disconnect and
      // advertise again. Otherwise they won't be in the list of discovered objects if we do
      // the discovery phase right away.
      _lastConnectivityCheckTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      break;
    }
      
    default:
    {
      PRINT_NAMED_ERROR("BlockFilter::HandleGameEvents unexpected message","%hhu",event.GetData().GetTag());
    }
  }
}

void BlockFilter::SendBlockPoolData() const
{
  PRINT_CH_INFO("BlockPool", "BlockFilter.HandleGameEvents", "Sending blockpool data");

  ExternalInterface::BlockPoolDataMessage msg;
  msg.blockPoolEnabled = _enabled;
  
  // Add information about the persistent pool
  std::vector<ExternalInterface::BlockPoolBlockData> persistentPool;
  for (const ObjectInfo &objectInfo : _persistentPool)
  {
    if (objectInfo.factoryID != ActiveObject::InvalidFactoryID)
    {
        ExternalInterface::BlockPoolBlockData blockData;
        blockData.factoryID = objectInfo.factoryID;
        blockData.objectType = objectInfo.objectType;

        persistentPool.push_back(blockData);
    }
  }

  msg.blockData = persistentPool;
  _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
}

} // namespace Cozmo
} // namespace Anki
