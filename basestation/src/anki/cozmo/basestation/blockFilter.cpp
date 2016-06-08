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

#include "anki/cozmo/basestation/blocks/blockFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "util/logging/logging.h"

#include <util/helpers/includeFstream.h>
//
#include <sys/stat.h>


// WORKAROUND: For some reason objects in slot 0 display lights incorrectly.
//             For now, reserving slot 0 for charger because we care less about its lights.
//             The real fix belongs in robot body firmware.
#define RESERVE_SLOT0_FOR_CHARGER 1


namespace Anki {
namespace Cozmo {
    
BlockFilter::BlockFilter(Robot* inRobot)
  : _robot(inRobot)
  , _path()
  , _enabled(false)
  , _externalInterface(nullptr)
{
  _blocks.fill(0);
}

BlockFilter::BlockFilter(const std::string &path, Robot* inRobot)
  : _robot(inRobot)
  , _blocks()
  , _path(path)
  , _enabled(false)
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
    
    if (_externalInterface != nullptr) {
      auto callback = std::bind(&BlockFilter::HandleGameEvents, this, std::placeholders::_1);
      _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));
      _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage, callback));
      _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage, callback));
    }
  }
}

bool BlockFilter::ContainsFactoryId(FactoryID factoryId) const
{
  return !_enabled || (std::find(_blocks.begin(), _blocks.end(), factoryId) != std::end(_blocks));
}

void BlockFilter::AddFactoryId(const FactoryID factoryId)
{
  // Find an available slot
  auto startIter = std::begin(_blocks);

  if (RESERVE_SLOT0_FOR_CHARGER) {
    // If this is a charger,
    ObjectType objType = _robot->GetDiscoveredObjectType(factoryId);
    if (objType == ObjectType::Invalid) {
      PRINT_NAMED_WARNING("BlockFilter.AddFactoryId.InvalidType",
                          "How did we get here without the object being a valid discovered object?");
    } else if (objType != ObjectType::Charger_Basic) {
      ++startIter;
    }
  }
  
  FactoryIDArray::iterator it = std::find(startIter, std::end(_blocks), 0);
  if (it != _blocks.end()) {
    *it = factoryId;
    ConnectToBlocks();
  } else {
    PRINT_NAMED_ERROR("BlockFilter.AddFactoryId", "There is no available slot to add factory ID %#08x", factoryId);
  }
}

void BlockFilter::RemoveFactoryId(const FactoryID factoryId)
{
  FactoryIDArray::iterator it = std::find(std::begin(_blocks), std::end(_blocks), factoryId);
  if (it != _blocks.end()) {
    *it = 0;
    ConnectToBlocks();
  } else {
    PRINT_NAMED_ERROR("BlockFilter.RemoveFactoryId", "Coudn't find factory ID %#08x", factoryId);
  }
}

void BlockFilter::RemoveAllFactoryIds()
{
  _blocks.fill(0);
  ConnectToBlocks();
}

bool BlockFilter::Save(const std::string &path) const
{
  // Find if there is any block connnected
  FactoryIDArray::const_iterator it = std::find_if(std::begin(_blocks), std::end(_blocks), [](FactoryID id) {
    return id != 0;
  });
  
  if (it == std::end(_blocks))
  {
    // list is empty, delete the file if is exists
    struct stat buf;
    int result = 0;
    if (stat(path.c_str(), &buf) == 0)
    {
      result = remove(path.c_str());
    }
    return (result == 0);
  }

  std::ofstream outputFileSteam;
  outputFileSteam.exceptions(~std::ofstream::goodbit);
  try
  {
    outputFileSteam.open(path);
    for (FactoryIDArray::const_iterator it = std::begin(_blocks); it != std::end(_blocks); ++it)
    {
      const FactoryID &factoryId = *it;
      if (factoryId != 0 || (RESERVE_SLOT0_FOR_CHARGER && it == _blocks.begin())) {
        outputFileSteam << "0x";
        outputFileSteam << std::hex << factoryId;
        outputFileSteam << "\n";
        PRINT_NAMED_DEBUG("BlockFilter.Save", "%#08x", factoryId);
      }
    }
    outputFileSteam.close();
  }
  catch (std::ofstream::failure e)
  {
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
  std::ifstream inputFileSteam;
  inputFileSteam.open(path);
  if (!inputFileSteam.good())
  {
    inputFileSteam.close();
    return false;
  }

  int lineIndex = 0;
  std::string line;
  while (std::getline(inputFileSteam, line))
  {
    if (line.length() == 0)
      continue;
    
    if (lineIndex > (int)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS) {
      PRINT_NAMED_ERROR("BlockFilter.Load", "Found more than %d lines in the file. They will be ignored", (int)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS);
      break;
    }

    if (line.compare(0,2,"0x") == 0)
    {
      try
      {
        FactoryID v = (FactoryID)std::stoul(line, nullptr, 16);
        _blocks[lineIndex] = v;
        ++lineIndex;
        PRINT_NAMED_DEBUG("BlockFilter.Load", "%#08x", v);
      }
      catch (std::exception e)
      {
        PRINT_NAMED_DEBUG("BlockFilter.Load.ParseError", "%s", line.c_str());
      }
    }
  }

  inputFileSteam.close();

  ConnectToBlocks();
  
  return true;
}
 
bool BlockFilter::ConnectToBlocks() const
{
  if (_robot) {
    if( _robot->ConnectToObjects(_blocks) == RESULT_OK )
    {
      Save();
      return true;
    }
    return false;
  } else {
    PRINT_NAMED_WARNING("BlockFilter.ConnectToBlocks.NoRobot", "");
    return false;
  }
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
      for (const FactoryID &factoryId : _blocks ) {
        if (factoryId != 0) {
          ExternalInterface::BlockPoolBlockData blockData;
          blockData.enabled = true;
          blockData.factory_id = factoryId;
          allBlocks.push_back(blockData);
        }
      }
      // TODO: push in all discovered by robot but unlisted blocks here as well.
      
      msg.blockData = allBlocks;
      
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
      if (msg.selected)
      {
        AddFactoryId( msg.factoryId );
      }
      else
      {
        RemoveFactoryId(msg.factoryId);
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
