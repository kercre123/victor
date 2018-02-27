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

#include "coretech/common/engine/exceptions.h"


namespace Anki {
namespace Cozmo {
namespace QuadTreeTypes {
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NodeContent::NodeContent(const MemoryMapData& m)
: data(m.Clone()) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NodeContent::NodeContent(MemoryMapTypes::MemoryMapDataPtr m)
: data(m) {}
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator==(const NodeContent& other) const
{
  return (data == other.data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator!=(const NodeContent& other) const
{
  const bool ret = !(this->operator==(other));
  return ret;
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
