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

#include "util/logging/logging.h"

#include <util/helpers/includeSstream.h>
#include <util/helpers/includeFstream.h>

#include <sys/stat.h>
#include <cstdio>
#include <string>
#include <sstream>

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
{
}

BlockFilter::BlockFilter(const std::string &path)
  : _blocks()
  , _path(path)
  , _enabled(true)
{
  Load(_path);
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
  
  if( _path.empty() )
  {
    _path = path;
  }

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

} // namespace Cozmo
} // namespace Anki
