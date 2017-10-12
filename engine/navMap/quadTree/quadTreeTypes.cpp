/**
 * File: quadTreeTypes.cpp
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Type definitions for QuadTree.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "quadTreeTypes.h"

#include "engine/navMap/memoryMap/data/memoryMapData.h"

#include "anki/common/basestation/exceptions.h"


namespace Anki {
namespace Cozmo {
namespace QuadTreeTypes {
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NodeContent::NodeContent(ENodeType t, const MemoryMapData& m)
: type(t)
, data(m.Clone()) {}
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator==(const NodeContent& other) const
{
  const bool sameType = ( type == other.type ) && ( data->type == other.data->type );
  
  if (sameType) 
  {
    const bool equals =    
      (data == other.data) || 
      ((data != nullptr) && (other.data != nullptr) && (data->Equals(other.data.get())));      
    return equals;
  }
  else 
  {
    return false;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator!=(const NodeContent& other) const
{
  const bool ret = !(this->operator==(other));
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ENodeTypeToString(ENodeType nodeType)
{
  switch (nodeType) {
    case ENodeType::Invalid: return "Invalid";
    case ENodeType::Subdivided: return "Subdivided";
    case ENodeType::Leaf: return "Leaf";
    case ENodeType::_Count: return "ERROR_COUNT_SHOULD_NOT_BE_USED";
  }
  return "ERROR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* EDirectionToString(EDirection dir)
{
  switch (dir) {
    case QuadTreeTypes::EDirection::North:   { return "N"; };
    case QuadTreeTypes::EDirection::East:    { return "E"; };
    case QuadTreeTypes::EDirection::South:   { return "S"; };
    case QuadTreeTypes::EDirection::West:    { return "W"; };
    case QuadTreeTypes::EDirection::Invalid: { return "Invalid"; };
  }
  return "Error";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec3f EDirectionToNormalVec3f(EDirection dir)
{
  switch (dir) {
    case QuadTreeTypes::EDirection::North:   { return Vec3f{ 1.0f,  0.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::East:    { return Vec3f{ 0.0f, -1.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::South:   { return Vec3f{-1.0f,  0.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::West:    { return Vec3f{ 0.0f,  1.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::Invalid: {};
  }
  
  DEV_ASSERT(!"Invalid direction", "EDirectionToNormalVec3f.InvalidDirection");
  return Vec3f{0.0f, 0.0f, 0.0f};
}

} // namespace
} // namespace
} // namespace
