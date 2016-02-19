/**
 * File: blockFilter.cpp
 *
 * Author: Molly Jameson
 * Date:   2/18/2016
 *
 * Description: blockFilter is a serializable set of objectIDs.
 *                So Cozmo only uses certain blocks.
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "anki/cozmo/basestation/blocks/blockFilter.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "util/logging/logging.h"

#include <util/helpers/includeFstream.h>
//
#include <sys/stat.h>

namespace Anki {
namespace Cozmo {
    
BlockFilter::BlockFilter()
  : BlockFilter(ObjectIDSet())
{
}

BlockFilter::BlockFilter(const ObjectIDSet &vehicleIds)
  : _blocks()
  , _path()
  , _enabled(true)
  , _externalInterface(nullptr)
{
}

BlockFilter::BlockFilter(const std::string &path)
  : _blocks()
  , _path(path)
  , _enabled(true)
  , _externalInterface(nullptr)
{
  Load(_path);
}
  
void BlockFilter::Init(const std::string &path, IExternalInterface* externalInterface)
{
  if( _externalInterface == nullptr)
  {
    _path = path;
    _externalInterface = externalInterface;
    _enabled = Load(path);
    
    auto callback = std::bind(&BlockFilter::HandleGameEvents, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage, callback));
  }
}

void BlockFilter::GetObjectIds(ObjectIDSet &objectIds) const
{
  objectIds.insert(_blocks.begin(), _blocks.end());
}

bool BlockFilter::ContainsObjectId(ObjectID objectId) const
{
  return !_enabled || (_blocks.find(objectId) != std::end(_blocks));
}

void BlockFilter::AddObjectId(const ObjectID objectId)
{
  _blocks.insert(objectId);
}

void BlockFilter::AddObjectIds(const ObjectIDSet &objectIds)
{
  _blocks.insert(objectIds.begin(), objectIds.end());
}

void BlockFilter::RemoveObjectId(const ObjectID objectId)
{
  _blocks.erase(objectId);
}

void BlockFilter::RemoveObjectIds(const ObjectIDSet &objectIds)
{
  for (const ObjectID &objectId : objectIds) {
    _blocks.erase(objectId);
  }
}

void BlockFilter::RemoveAllObjectIds()
{
  _blocks.clear();
}

bool BlockFilter::Save(const std::string &path) const
{
  if (_blocks.empty()) {
    // list is empty, delete the file if is exists
    struct stat buf;
    int result = 0;
    if (stat(path.c_str(), &buf) == 0) {
      result = remove(path.c_str());
    }
    return (result == 0);
  }

  std::ofstream outputFileSteam;
  outputFileSteam.exceptions(~std::ofstream::goodbit);
  try {
    outputFileSteam.open(path);
    for (ObjectIDSet::const_iterator it = std::begin(_blocks);
         it != std::end(_blocks);
         ++it) {
      const ObjectID &vehicleId = *it;
      outputFileSteam << "0x";
      outputFileSteam << std::hex << vehicleId;
      outputFileSteam << "\n";
      PRINT_NAMED_DEBUG("BlockFilter.Save", "0x%d", vehicleId.GetValue());
    }
    outputFileSteam.close();
  } catch (std::ofstream::failure e) {
    PRINT_NAMED_ERROR("BlockFilter.Save.Error", "%s", e.std::exception::what());
    return false;
  }

  return true;
}

bool BlockFilter::Save() const
{
  if (_path.empty())
    return false;

  return Save(_path);
}

bool BlockFilter::Load(const std::string &path)
{
  ObjectIDSet blocks;

  std::ifstream inputFileSteam;
  inputFileSteam.open(path);
  if (!inputFileSteam.good()) {
    inputFileSteam.close();
    return false;
  }

  std::string line;
  while (std::getline(inputFileSteam, line)) {
    if (line.length() == 0)
      continue;

    if (line.compare(0,2,"0x") == 0) {
      try {
        ObjectID v = std::stoi(line, nullptr, 16);
        blocks.insert(v);
        PRINT_NAMED_DEBUG("BlockFilter.Load", "0x%d", v.GetValue());
      } catch (std::exception e) {
        PRINT_NAMED_DEBUG("BlockFilter.Load.ParseError", "%s", line.c_str());
      }
    }
  }

  _blocks = blocks;
  inputFileSteam.close();

  return true;
}
  
void BlockFilter::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    // Debug menu tab just opened, asking for the current state of things
    case ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage:
    {
      ExternalInterface::InitBlockPoolMessage msg;
      msg.blockPoolEnabled = _enabled;
      
      std::vector<ExternalInterface::BlockPoolBlockData> allBlocks;
      for (const ObjectID &objectId : _blocks ) {
        ExternalInterface::BlockPoolBlockData blockData;
        blockData.enabled = true;
        blockData.id = objectId.GetValue();
        allBlocks.push_back(blockData);
      }
      // TODO: push in all discovered by robot but unlisted blocks here as well.
      
      //msg.blockData = allBlocks;
      
      _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage:
    {
      const Anki::Cozmo::ExternalInterface::BlockPoolEnabledMessage& msg = event.GetData().Get_BlockPoolEnabledMessage();
      _enabled = msg.enabled;
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage:
    {
      const Anki::Cozmo::ExternalInterface::BlockSelectedMessage& msg = event.GetData().Get_BlockSelectedMessage();
      if( msg.selected != 0 )
      {
        AddObjectId( msg.blockId );
      }
      else
      {
        RemoveObjectId(msg.blockId);
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BlockFilter::HandleGameEvents unexpected message","%hhu",event.GetData().GetTag());
    }
  }
}

} // namespace Cozmo
} // namespace Anki
