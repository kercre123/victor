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
  : BlockFilter(FactoryIDSet())
{
}

BlockFilter::BlockFilter(const FactoryIDSet &factoryIds)
  : _blocks(factoryIds)
  , _path()
  , _enabled(false)
  , _externalInterface(nullptr)
{
}

BlockFilter::BlockFilter(const std::string &path)
  : _blocks()
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
    
    auto callback = std::bind(&BlockFilter::HandleGameEvents, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockPoolEnabledMessage, callback));
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::BlockSelectedMessage, callback));
  }
}

void BlockFilter::GetFactoryIds(FactoryIDSet &factoryIds) const
{
  factoryIds.insert(_blocks.begin(), _blocks.end());
}

bool BlockFilter::ContainsFactoryId(FactoryID factoryId) const
{
  return !_enabled || (_blocks.find(factoryId) != std::end(_blocks));
}

void BlockFilter::AddFactoryId(const FactoryID factoryId)
{
  _blocks.insert(factoryId);
}

void BlockFilter::AddFactoryIds(const FactoryIDSet &factoryIds)
{
  _blocks.insert(factoryIds.begin(), factoryIds.end());
}

void BlockFilter::RemoveFactoryId(const FactoryID factoryId)
{
  _blocks.erase(factoryId);
}

void BlockFilter::RemoveFactoryIds(const FactoryIDSet &factoryIds)
{
  for (const FactoryID &factoryID : factoryIds)
  {
    _blocks.erase(factoryID);
  }
}

void BlockFilter::RemoveAllFactoryIds()
{
  _blocks.clear();
}

bool BlockFilter::Save(const std::string &path) const
{
  if (_blocks.empty())
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
    for (FactoryIDSet::const_iterator it = std::begin(_blocks); it != std::end(_blocks); ++it)
    {
      const FactoryID &factoryId = *it;
      outputFileSteam << "0x";
      outputFileSteam << std::hex << factoryId;
      outputFileSteam << "\n";
      PRINT_NAMED_DEBUG("BlockFilter.Save", "%#08x", factoryId);
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
  FactoryIDSet blocks;

  std::ifstream inputFileSteam;
  inputFileSteam.open(path);
  if (!inputFileSteam.good())
  {
    inputFileSteam.close();
    return false;
  }

  std::string line;
  while (std::getline(inputFileSteam, line))
  {
    if (line.length() == 0)
      continue;

    if (line.compare(0,2,"0x") == 0)
    {
      try
      {
        FactoryID v = (FactoryID)std::stoul(line, nullptr, 16);
        blocks.insert(v);
        PRINT_NAMED_DEBUG("BlockFilter.Load", "%#08x", v);
      }
      catch (std::exception e)
      {
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
      for (const FactoryID &factoryId : _blocks ) {
        ExternalInterface::BlockPoolBlockData blockData;
        blockData.enabled = true;
        blockData.id = factoryId;
        allBlocks.push_back(blockData);
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
      if( msg.selected != 0 )
      {
        AddFactoryId( msg.blockId );
      }
      else
      {
        RemoveFactoryId(msg.blockId);
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
